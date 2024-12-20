#include "stdlib.h"
#include "stdio.h"
#include <fstream>
#include <assert.h>
#include <cstring>
#include <atomic>
#include <thread>
// websocket库
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
// #include <unistd.h>
#include "httplib.h"
#include "json.hpp"
#include<Windows.h>

#include "../include/aikit_biz_api.h"
#include "../include/aikit_constant.h"
#include "../include/aikit_biz_config.h"
#include "../include/aikit_biz_builder.h"

#include "transcode.h"

#pragma comment(lib, "winmm.lib")  

#define FRAME_LEN	320 //16k采样率的16bit音频，一帧的大小为640B, 时长20ms

using namespace std;
using namespace AIKIT;

static const char* ABILITY = "e867a88f2";

// ------ websocket代码 start -------
typedef websocketpp::server<websocketpp::config::asio> server;

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

// pull out the type of messages sent by our config
typedef server::message_ptr message_ptr;

// 存储连接句柄
websocketpp::connection_hdl connection;
// 存储server句柄
server* global_server = nullptr;

// 连接打开处理器
void on_open(server* s, websocketpp::connection_hdl hdl) {
	// 保存连接句柄
	connection = hdl;
	// 保存server
	global_server = s;
	std::cout << "连接打开: " << hdl.lock().get() << std::endl;
}

// 连接关闭处理器
void on_close(server* s, websocketpp::connection_hdl hdl) {
	std::cout << "连接关闭: " << hdl.lock().get() << std::endl;
}

// Define a callback to handle incoming messages
void on_message(server* s, websocketpp::connection_hdl hdl, message_ptr msg) {
	std::cout << "on_message called with hdl: " << hdl.lock().get()
		<< " and message: " << msg->get_payload()
		<< std::endl;
	std::cout << "收到消息: " << msg->get_payload() << std::endl;

	// check for a special command to instruct the server to stop listening so
	// it can be cleanly exited.
	if (msg->get_payload() == "stop-listening") {
		s->stop_listening();
		return;
	}

	try {
		s->send(hdl, msg->get_payload(), msg->get_opcode());
	}
	catch (websocketpp::exception const& e) {
		std::cout << "Echo failed because: "
			<< "(" << e.what() << ")" << std::endl;
	}
}
// ------ websocket代码 end -------

std::map<std::string, std::string> keywordToUrlMap;

// 文件读取keyword映射配置
bool loadKeywordToUrlMap(const std::string& filename, std::map<std::string, std::string>& map) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string keyword, url;

		// 使用逗号作为分隔符
		if (std::getline(iss, keyword, ',') && std::getline(iss, url)) {
			map[keyword] = url;
		}
		else {
			std::cerr << "无效的行格式: " << line << std::endl;
		}
	}

	file.close();
	return true;
}


// 封装的 utf8ToGbk 函数
std::string utf8ToGbk(const std::string& utf8Str) {
	// 计算 UTF-8 字符串的长度
	int utf8Length = utf8Str.length();

	// 分配足够的内存来存储 GBK 编码的字符串
	// 假设每个 GBK 字符最多占用 2 字节
	int maxGbkLength = utf8Length * 2;
	char* gbkStr = new char[maxGbkLength];

	// 调用 utf8_to_gbk 函数进行编码转换
	int gbkLength = utf8_to_gbk(const_cast<char*>(utf8Str.c_str()), utf8Length, gbkStr, maxGbkLength);

	// 将转换后的 GBK 字符串存储回 std::string
	std::string gbkKeyword(gbkStr, gbkLength);

	// 释放分配的内存
	delete[] gbkStr;

	return gbkKeyword;
}

void OnOutput(AIKIT_HANDLE* handle, const AIKIT_OutputData* output) {
	printf(">>>>>> 监听到唤醒词......\n");
	// 打印能力ID和键
	printf("OnOutput abilityID :%s\n", handle->abilityID);
	printf("OnOutput key:%s\n", output->node->key);

	int len = output->node->len;
	std::unique_ptr<char[]> value(new char[len + 1]);
	memset(value.get(), 0, len + 1);
	// 直接复制 UTF-8 字符串
	strncpy(value.get(), (char*)output->node->value, len);
	int ret_len = len; // 返回实际复制的长度
	printf("OnOutput value:%s\n", value.get());

	try {
		// 解析JSON串获取到keyword字段的值
		nlohmann::json jsonData = nlohmann::json::parse(value.get());
		std::string keyword = jsonData["rlt"][0]["keyword"];
		// 输出解析的keyword
		std::cout << ">>>>>> Keyword (UTF-8): " << keyword << std::endl;
		// 调用封装的方法进行编码转换
		std::string gbkKeyword = utf8ToGbk(keyword);
		std::cout << ">>>>>> Keyword (GBK): " << gbkKeyword << std::endl;
		// 根据关键字获取URL
		if (keywordToUrlMap.find(keyword) != keywordToUrlMap.end()) {
			// 将keyword转为GBK编码
			std::string value = keywordToUrlMap[keyword];
			// 向web端发生数据
			std::cout << "向浏览器端发送数据：" << value << endl;
			global_server->send(connection, value, websocketpp::frame::opcode::text);
		}
		else {
			std::cerr << "No value found for keyword: " << gbkKeyword << "\n";
		}
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "JSON parse error: " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "发送数据失败: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown exception occurred.\n";
	}
}

void OnOutput_http_requst(AIKIT_HANDLE* handle, const AIKIT_OutputData* output) {
	// 打印能力ID和键
	printf("OnOutput abilityID :%s\n", handle->abilityID);
	printf("OnOutput key:%s\n", output->node->key);

	// 获取数据
	int len = output->node->len;
	std::unique_ptr<char[]> value(new char[len + 1]);
	memset(value.get(), 0, len + 1);
	//int ret_len = utf8_to_gbk((char*)output->node->value, len, value.get(), len);
	// 直接复制 UTF-8 字符串
	strncpy(value.get(), (char*)output->node->value, len);
	//value.get()[len] = '\0'; // 确保字符串以 null 结尾
	int ret_len = len; // 返回实际复制的长度
	printf("OnOutput value:%s\n", value.get());

	try {
		// 解析JSON串获取到keyword字段的值
		nlohmann::json jsonData = nlohmann::json::parse(value.get());
		std::string keyword = jsonData["rlt"][0]["keyword"];
		// 输出解析的keyword
		std::cout << "Keyword (UTF-8): " << keyword << std::endl;
		// 将keyword转为GBK
		// - 计算 UTF-8 字符串的长度
		int utf8_length = keyword.length();

		// - 分配足够的内存来存储 GBK 编码的字符串
		// - 假设每个 GBK 字符最多占用 2 字节
		int max_gbk_length = utf8_length * 2;
		char* gbk_str = new char[max_gbk_length];

		// - 调用 utf8_to_gbk 函数进行编码转换
		int gbk_length = utf8_to_gbk(const_cast<char*>(keyword.c_str()), utf8_length, gbk_str, max_gbk_length);
		// 将转换后的 GBK 字符串存储回 std::string
		std::string gbk_keyword(gbk_str, gbk_length);
		std::cout << "Keyword (GBK): " << gbk_keyword << std::endl;

		// - 释放分配的内存
		delete[] gbk_str;
		// 根据关键字获取URL
		if (keywordToUrlMap.find(gbk_keyword) != keywordToUrlMap.end()) {
			// 将keyword转为GBK编码
			std::string url = keywordToUrlMap[gbk_keyword];

			// 调用HTTP接口
			httplib::Client cli("http://10.132.19.87:18000");
			auto res = cli.Get(url.c_str());

			if (res && res->status == 200) {
				std::cout << "HTTP Response Status: " << res->status << "\n";
				std::cout << "HTTP Response Body: " << res->body << "\n";
			}
			else {
				std::cerr << "HTTP Request Failed. Status Code: ";
				if (res) {
					std::cerr << res->status << "\n";
				}
				else {
					std::cerr << "No response received.\n";
				}
			}
		}
		else {
			std::cerr << "No URL found for keyword: " << gbk_keyword << "\n";
		}
	}
	catch (const nlohmann::json::parse_error& e) {
		std::cerr << "JSON parse error: " << e.what() << "\n";
	}
	catch (const std::exception& e) {
		std::cerr << "Exception occurred during HTTP request: " << e.what() << "\n";
	}
	catch (...) {
		std::cerr << "Unknown exception occurred during HTTP request.\n";
	}
}

void OnEvent(AIKIT_HANDLE* handle, AIKIT_EVENT eventType, const AIKIT_OutputEvent* eventValue) {
	printf("OnEvent:%d\n", eventType);

}

void OnError(AIKIT_HANDLE* handle, int32_t err, const char* desc) {
	printf("OnError:%d\n", err);
}

void OnMM_WIM_DATA(HWAVEIN* _hWaveIn, WAVEHDR* _wHdr)//录音完成
{
	HWAVEIN hWaveIn = *_hWaveIn;

	//释放录音缓冲区
	waveInUnprepareHeader(hWaveIn, _wHdr, sizeof(WAVEHDR));
	//重新准备缓冲区
	waveInPrepareHeader(hWaveIn, _wHdr, sizeof(WAVEHDR));
	//重新加入缓冲区
	waveInAddBuffer(hWaveIn, _wHdr, sizeof(WAVEHDR));
}

int ivw_microphone(int* index)
{
	int ret = 0;
	AIKIT_DataBuilder* dataBuilder = nullptr;
	AIKIT_ParamBuilder* paramBuilder = nullptr;
	AIKIT_HANDLE* handle = nullptr;
	AiAudio* aiAudio_raw = nullptr;
	DWORD bufsize;
	int len = 0;
	int audio_count = 0;
	int count = 0;

	HWAVEIN hWaveIn = nullptr;  //输入设备
	WAVEFORMATEX waveform; //采集音频的格式，结构体
	BYTE* pBuffer = nullptr;//采集音频时的数据缓存
	WAVEHDR wHdr; //采集音频时包含数据缓存的结构体
	HANDLE          wait;

	waveform.wFormatTag = WAVE_FORMAT_PCM;//声音格式为PCM
	waveform.nSamplesPerSec = 16000;//音频采样率
	waveform.wBitsPerSample = 16;//采样比特
	waveform.nChannels = 1;//采样声道数
	waveform.nAvgBytesPerSec = 16000;//每秒的数据率
	waveform.nBlockAlign = 2;//一个块的大小，采样bit的字节数乘以声道数
	waveform.cbSize = 0;//一般为0

	wait = CreateEvent(NULL, 0, 0, NULL);
	//使用waveInOpen函数开启音频采集
	waveInOpen(&hWaveIn, WAVE_MAPPER, &waveform, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);

	bufsize = 1024 * 500;//开辟适当大小的内存存储音频数据，可适当调整内存大小以增加录音时间，或采取其他的内存管理方案

	paramBuilder = AIKIT_ParamBuilder::create();
	ret = AIKIT_SpecifyDataSet(ABILITY, "key_word", index, 1);
	printf("AIKIT_SpecifyDataSet:%d\n", ret);
	if (ret != 0) {
		goto exit;
	}
	paramBuilder->param("wdec_param_nCmThreshold", "0 0:999", strlen("0 0:999"));
	paramBuilder->param("gramLoad", true);

	ret = AIKIT_Start(ABILITY, AIKIT_Builder::build(paramBuilder), nullptr, &handle);
	printf("AIKIT_Start:%d\n", ret);
	if (ret != 0) {
		return ret;
	}

	pBuffer = (BYTE*)malloc(bufsize);
	wHdr.lpData = (LPSTR)pBuffer;
	wHdr.dwBufferLength = bufsize;
	wHdr.dwBytesRecorded = 0;
	wHdr.dwUser = 0;
	wHdr.dwFlags = 0;
	wHdr.dwLoops = 1;
	waveInPrepareHeader(hWaveIn, &wHdr, sizeof(WAVEHDR));//准备一个波形数据块头用于录音
	waveInAddBuffer(hWaveIn, &wHdr, sizeof(WAVEHDR));//指定波形数据块为录音输入缓存
	waveInStart(hWaveIn);//开始录音
	printf(">>>>>> 正在录音......\n");

	dataBuilder = AIKIT_DataBuilder::create();
	//while (audio_count< bufsize && wakeupFlage!=1)//单次唤醒
	while (audio_count < bufsize)//持续唤醒
	{
		Sleep(100);//等待声音录制
		len = 10 * FRAME_LEN; //16k音频，10帧 （时长200ms）

		if (audio_count >= wHdr.dwBytesRecorded)
		{
			len = 0;
		}

		//printf(">>>>>>>>count=%d\n", count++);
		dataBuilder->clear();
		aiAudio_raw = AiAudio::get("wav")->data((const char*)&pBuffer[audio_count], len)->valid();
		dataBuilder->payload(aiAudio_raw);
		ret = AIKIT_Write(handle, AIKIT_Builder::build(dataBuilder));
		if (ret != 0) {
			printf("AIKIT_Write:%d\n", ret);
			//goto  exit;
			// 继续尝试不中断程序
		}
		audio_count += len;

		if (audio_count >= bufsize) {
			waveInStop(hWaveIn);
			waveInReset(hWaveIn); // 停止录音

			wHdr.dwBytesRecorded = 0;
			wHdr.dwUser = 0;
			wHdr.dwFlags = 0;
			wHdr.dwLoops = 0; // 不需要循环播放

			ret = waveInUnprepareHeader(hWaveIn, &wHdr, sizeof(WAVEHDR)); // 取消准备波形数据块头
			if (ret != MMSYSERR_NOERROR) {
				std::cerr << "Failed to unprepare wave header." << std::endl;
				// 继续尝试，不中断程序
			}

			ret = waveInPrepareHeader(hWaveIn, &wHdr, sizeof(WAVEHDR)); // 准备一个波形数据块头用于录音
			if (ret != MMSYSERR_NOERROR) {
				std::cerr << "Failed to prepare wave header." << std::endl;
				// 继续尝试，不中断程序
			}

			ret = waveInAddBuffer(hWaveIn, &wHdr, sizeof(WAVEHDR)); // 指定波形数据块为录音输入缓存
			if (ret != MMSYSERR_NOERROR) {
				std::cerr << "Failed to add wave buffer." << std::endl;
				// 继续尝试，不中断程序
			}

			waveInStart(hWaveIn); // 开始录音
			audio_count = 0;
		}
	}

exit:
	if (handle != nullptr)
		AIKIT_End(handle);
	if (hWaveIn != NULL)
	{
		waveInStop(hWaveIn);
		waveInReset(hWaveIn);//停止录音
		waveInClose(hWaveIn);
		hWaveIn = nullptr;
	}
	if (NULL != pBuffer)
	{
		free(pBuffer);
	}
	if (dataBuilder != nullptr) {
		delete dataBuilder;
		dataBuilder = nullptr;
	}

	return ret;
}

void TestIvw70() {
	AIKIT_OutputData* output = nullptr;
	int ret = 0;
	int times = 1;
	int index[] = { 0 };

	ret = AIKIT_EngineInit(ABILITY, nullptr);
	if (ret != 0) {
		printf("AIKIT_EngineInit failed:%d\n", ret);
		return;
	}

	if (times == 1) {
		AIKIT_CustomData customData;
		customData.key = "key_word";
		customData.index = 0;
		customData.from = AIKIT_DATA_PTR_PATH;
		customData.value = (void*)".\\resource\\ivw70\\xbxb.txt";
		customData.len = strlen(".\\resource\\ivw70\\xbxb.txt");
		customData.next = nullptr;
		customData.reserved = nullptr;
		printf("AIKIT_LoadData start!\n");
		ret = AIKIT_LoadData(ABILITY, &customData);
		printf("AIKIT_LoadData end!\n");
		printf("AIKIT_LoadData:%d\n", ret);
		if (ret != 0) {
			goto  exit;
		}
		times++;
	}

	printf("》》》》》》》正在读取keywordMaps.txt配置文件......\n");
	// 读取keywordMaps.txt配置文件
	if (!loadKeywordToUrlMap("keywordMaps.txt", keywordToUrlMap)) {
		std::cerr << "无法加载keywordMaps配置文件......" << std::endl;
	}
	else {
		// 打印加载的映射文件的内容
		for (const auto& pair : keywordToUrlMap) {
			std::cout << "key(GBK): " << utf8ToGbk(pair.first) << ", vaule: " << pair.second << std::endl;
		}
	}
	printf("》》》》》》》读取keywordMaps.txt配置文件成功！\n");
	ivw_microphone(index);
exit:

	AIKIT_EngineUnInit(ABILITY);
}
// 启动WebSocket服务
void run_websocket_server() {
	// Create a server endpoint
	server echo_server;

	try {
		// Set logging settings
		echo_server.set_access_channels(websocketpp::log::alevel::all);
		echo_server.clear_access_channels(websocketpp::log::alevel::frame_payload);

		// Initialize Asio
		echo_server.init_asio();

		// Register our message handler
		echo_server.set_message_handler(bind(&on_message, &echo_server, ::_1, ::_2));
		echo_server.set_open_handler(bind(&on_open, &echo_server, ::_1));
		echo_server.set_close_handler(bind(&on_close, &echo_server, ::_1));

		// Listen on port 9002
		echo_server.listen(9002);

		// Start the server accept loop
		echo_server.start_accept();

		// Start the ASIO io_service run loop
		echo_server.run();
	}
	catch (websocketpp::exception const& e) {
		std::cout << e.what() << std::endl;
	}
	catch (...) {
		std::cout << "other exception" << std::endl;
	}
	// 清理全局服务器句柄
	global_server = nullptr;

}

// 加载 config.txt 文件中的配置信息
bool loadConfigFromFile(const std::string& filename, char*& appID, char*& apiSecret, char*& apiKey) {
	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "无法打开文件: " << filename << std::endl;
		return false;
	}

	std::string line;
	while (std::getline(file, line)) {
		std::istringstream iss(line);
		std::string key, value;

		// 使用等号作为分隔符
		if (std::getline(iss, key, '=') && std::getline(iss, value)) {
			if (key == "appID") {
				appID = strdup(value.c_str());
			}
			else if (key == "apiSecret") {
				apiSecret = strdup(value.c_str());
			}
			else if (key == "apiKey") {
				apiKey = strdup(value.c_str());
			}
			else {
				std::cerr << "未知的配置项: " << key << std::endl;
			}
		}
		else {
			std::cerr << "无效的行格式: " << line << std::endl;
		}
	}

	file.close();
	return true;
}

int main() {
	// 启动一个新的线程来运行WebSocket服务器
	std::thread websocket_thread(run_websocket_server);
	websocket_thread.detach(); // 分离线程，使其独立运行
	// 语音唤醒配置
	char* appID = nullptr;
	char* apiSecret = nullptr;
	char* apiKey = nullptr;
	// 读取配置文件
	printf("》》》》》》》正在读取config.txt配置文件......\n");
	if (!loadConfigFromFile("config.txt", appID, apiSecret, apiKey)) {
		std::cerr << "无法加载 config.txt 文件" << std::endl;
		return -1;
	}
	// 打印加载的配置信息
	std::cout << "App ID: " << appID << std::endl;
	std::cout << "API Secret: " << apiSecret << std::endl;
	std::cout << "API Key: " << apiKey << std::endl;
	AIKIT_Configurator::builder()
		.app()
		.appID(appID)
		.apiSecret(apiSecret)
		.apiKey(apiKey)
		.workDir(".\\")
		.auth()
		.authType(0)
		.ability(ABILITY)
		.log()
		.logMode(2)
		.logPath(".\\aikit.log");
	// 释放内存
	free(appID);
	free(apiSecret);
	free(apiKey);
	printf("》》》》》》》读取config.txt配置文件成功！\n");

	int ret = AIKIT_Init();
	if (ret != 0) {
		printf("AIKIT_Init failed:%d\n", ret);
		return -1;
	}
	AIKIT_Callbacks cbs = { OnOutput,OnEvent,OnError };
	AIKIT_RegisterAbilityCallback(ABILITY, cbs);

	TestIvw70();

	system("pause");
	AIKIT_UnInit();
	return 0;
}
