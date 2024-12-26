# AIkit_awaken_windows_web_tools

## 一、介绍

`AIkit_awaken_windows_web_tools` 是一个基于科大讯飞语音唤醒 SDK 和 WebSocket++ 库的 Windows 工具。该项目旨在提供一个集成语音唤醒和 Web 通信功能的应用程序，可实现 web 端的语音唤醒功能。例如，语音输入“打开智能问答窗口”，该工具可识别配置的“智能问答”（key）唤醒词，然后向浏览器端发送消息“znwd”(value)，浏览器可以监听该消息并弹出智能问答窗口。适用于 Windows 操作系统。

## 二、基础环境

1. **科大讯飞语音唤醒 Windows SDK 文档**
   - **文档地址**: [科大讯飞语音唤醒 Windows SDK](https://www.xfyun.cn/doc/asr/AIkit_awaken/Windows-SDK.html)
   - **说明**: 请参考官方文档进行 SDK 的安装和配置。

2. **C++ WebSocket Client/Server Library (WebSocket++)**
   - **GitHub 地址**: [WebSocket++](https://github.com/zaphoyd/websocketpp)
   - **安装配置参考**: 
     - [Windows 下编译和使用 WebSocket++](https://www.cnblogs.com/RioTian/p/17615409.html)

3. **Boost (WebSocket 依赖 Boost 环境)**
   - **资源下载**: 
     - [Boost 下载和安装指南](https://www.cnblogs.com/RioTian/p/17581582.html)
   - **说明**: WebSocket++ 依赖于 Boost 库，请按照指南完成 Boost 的安装和配置。

## 三、开发工具

- **Visual Studio 2022**
  - **说明**: 使用 Visual Studio 2022 进行项目的开发和调试。

## 四、测试
- **用Visual Studio 2022 打开源码运行或者直接运行编译打包好的: [release.zip](https://github.com/Glong97/aikit_awaken_windows_web_tools/releases/tag/v1.0.0)**

1. **运行程序**
   - **步骤**:
     1. 打开 Visual Studio 2022。
     2. 加载项目文件（`.sln`）。
     3. 将语音唤醒 Windows SDK环境、websocketpp、boost的环境配置到vs工程的附加包含目录和库目录中。
     4. 配置`bin\config.txt`，输入语音唤醒的`appID`、`apiSecret`、`apiKey`（去科大讯飞官网获取可免费使用一个月）。
     5. 配置`bin\resource\ivw70\xbxb.txt`，配置语音唤醒词。
     6. 配置`bin\keywordMaps.txt`，输入唤醒词（key）和要发送的数据（value）;
     7. 运行程序。

2. **打开 `websocket_test.html`**
   - **步骤**:
     1. 确保服务器端程序正在运行。
     2. 双击打开 `websocket_test.html`
     3. 测试 WebSocket 连接和消息传递功能。

## 五、运行效果

![result.png](https://github.com/Glong97/aikit_awaken_windows_web_tools/blob/master/imgs/result.png)
