#pragma once

#if defined _WIN32 || defined _WIN64 || defined _MSC_VER
int gbk_to_utf8(char* str_GBK, int length, char* out_str, int out_len);
int utf8_to_gbk(char* str_UTF8, int length, char* out_str, int out_len);

#elif defined __GNUC__ && defined __unix__
size_t gbk_to_utf8(char* gbk_str, size_t in_len, char* out_str, size_t out_len);
size_t utf8_to_gbk(char* utf8_str, size_t in_len, char* out_str, size_t out_len);
#endif

