#pragma once

#include "Libraries/WindowsApi.hpp"

#if defined(CHALET_WIN32)
	#if defined(UNICODE)
		#define TO_WIDE(x, ...) String::toWideString(x, __VA_ARGS__)
		#define FROM_WIDE(x, ...) String::fromWideString(x, __VA_ARGS__)
		#define TSTRING std::wstring
	#else
		#define TO_WIDE(x, ...) x
		#define FROM_WIDE(x, ...) x
		#define TSTRING std::string
	#endif
#endif
