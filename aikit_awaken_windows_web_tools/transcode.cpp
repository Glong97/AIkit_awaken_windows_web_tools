#if defined _WIN32 || defined _WIN64 || defined _MSC_VER
#include <stdio.h>
#include <windows.h>
#include "transcode.h"
int gbk_to_utf8(char* str_GBK, int length, char* out_str, int out_len)
{
	if (str_GBK == NULL || out_str == NULL)
	{
		printf("input str is empty\n");
		return -1;
	}
	int len = MultiByteToWideChar(CP_ACP, 0, str_GBK, length, NULL, 0);
	wchar_t* wstr = (wchar_t*)malloc(sizeof(wchar_t) * len);
	memset(wstr, 0, len * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, str_GBK, length, wstr, len);
	int lenUTF8 = WideCharToMultiByte(CP_UTF8, 0, wstr, len, NULL, 0, NULL, NULL);
	memset(out_str, 0, out_len);
	WideCharToMultiByte(CP_UTF8, 0, wstr, len, out_str, lenUTF8, NULL, NULL);
	if (wstr) free(wstr);
	return lenUTF8;
}

int utf8_to_gbk(char* str_UTF8, int length, char* out_str, int out_len)
{
	if (str_UTF8 == NULL || out_str == NULL)
	{
		printf("input str is empty\n");
		return -1;
	}
	int len = MultiByteToWideChar(CP_UTF8, 0, str_UTF8, length, NULL, 0);
	wchar_t* wszGBK = (wchar_t*)malloc(sizeof(wchar_t) * len);
	memset(wszGBK, 0, len * sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, str_UTF8, length, wszGBK, len);
	int lenGBK = WideCharToMultiByte(CP_ACP, 0, wszGBK, len, NULL, 0, NULL, NULL);
	memset(out_str, 0, lenGBK);
	WideCharToMultiByte(CP_ACP, 0, wszGBK, len, out_str, lenGBK, NULL, NULL);
	if (wszGBK) free(wszGBK);
	return lenGBK;
}

#elif defined __GNUC__ && defined __unix__
#include <string.h>
#include <errno.h>
#include <iconv.h>
#include <stdio.h>
#include <malloc.h>
size_t gbk_to_utf8(char* gbk_str, size_t in_len, char* out_str, size_t out_len) {
	if (gbk_str == NULL || out_str == NULL)
	{
		printf("input str is empty\n");
		return -1;
	}
	iconv_t cd;
	char** pin = &gbk_str;
	char** pout = &out_str;
	char* temp = out_str;
	size_t len1 = in_len;
	size_t len2 = out_len;
	cd = iconv_open("UTF-8", "GBK");
	if (cd == (iconv_t)-1) {
		perror("iconv_open error");
		return -1;
	}

	if (iconv(cd, pin, &len1, pout, &len2) == (size_t)-1) {
		perror("iconv error");
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return strlen(temp);
}

size_t utf8_to_gbk(char* utf8_str, size_t in_len, char* out_str, size_t out_len) {
	if (utf8_str == NULL || out_str == NULL)
	{
		printf("input str is empty\n");
		return -1;
	}
	iconv_t cd;
	char** pin = &utf8_str;
	char** pout = &out_str;
	char* temp = out_str;
	size_t len1 = in_len;
	size_t len2 = out_len;
	cd = iconv_open("GBK", "UTF-8");
	if (cd == (iconv_t)-1) {
		perror("iconv_open error");
		return -1;
	}

	if (iconv(cd, pin, &len1, pout, &len2) == (size_t)-1) {
		perror("iconv error");
		iconv_close(cd);
		return -1;
	}
	iconv_close(cd);
	return strlen(temp);
}
#endif
//int main(int argc, char** argv) {
//	char utf8_str[] = { 0xe4,0xb8,0x80,0xe4,0xba,0x8c,0xe4,0xb8,0x89,0xe5,0x9b,0x9b,0xe4,0xba,0x94,0xef,0xbc,0x8c,0xe3,0x80,0x82,0xef,0xbc,0x81,0x09,0x26,0x25 };//一二三四五，。！\t&%
//	char gbk_str[] = { 0xd2,0xbb,0xb6,0xfe,0xc8,0xfd,0xcb,0xc4,0xce,0xe5,0xa3,0xac,0xa1,0xa3,0xa3,0xa1,0x09,0x26,0x25 };//一二三四五，。！\t&%
//	char cache[1024] = { 0 };
//	int ret_len = utf8_to_gbk(utf8_str, sizeof(utf8_str), cache, sizeof(cache));
//	printf("return len:%d,real len:%d\n", ret_len, sizeof(gbk_str));
//	if (strncmp(gbk_str, cache, ret_len) != 0) printf("translate result different\n");
//	else printf("UTF8 translate to GBK sucess\n");
//	memset(cache, 0, sizeof(cache));
//	ret_len = gbk_to_utf8(gbk_str, sizeof(gbk_str), cache, sizeof(cache));
//	printf("return len:%d,real len:%d\n", ret_len, sizeof(utf8_str));
//	if (strncmp(utf8_str, cache, ret_len) != 0) printf("translate result different\n");
//	else printf("GBK translate to UTF8 sucess\n");
//	printf("press any key to quit...\n");
//	getchar();
//	return 0;
//}