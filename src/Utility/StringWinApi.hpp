#pragma once

#include "Libraries/WindowsApi.hpp"

#if defined(CHALET_WIN32)
	#if defined(UNICODE)
		#define TO_WIDE(x, ...) String::toWideString(x, __VA_ARGS__)
		#define FROM_WIDE(x, ...) String::fromWideString(x, __VA_ARGS__)
		#define USTRING std::wstring
		#define WINSTR_CHAR WCHAR
		#define WINSTR_PTR LPWSTR
	#else
		#define TO_WIDE(x, ...) x
		#define FROM_WIDE(x, ...) x
		#define USTRING std::string
		#define WINSTR_CHAR CHAR
		#define WINSTR_PTR LPSTR
	#endif
#endif
