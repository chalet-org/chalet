/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#if defined(_MSC_VER) && _MSC_VER >= 1920
#elif defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
	#if __has_include(<filesystem>)
		#if __clang_major__ >= 11 || __GNUC__ >= 9
		#else
			#define CHALET_NO_FILE_SYSTEM
		#endif
	#else
		#define CHALET_NO_FILE_SYSTEM
	#endif
#else
	#define CHALET_NO_FILE_SYSTEM
#endif

#ifdef CHALET_NO_FILE_SYSTEM
	#error "Chalet requires a compiler with C++17 and std::filesystem"
#else
	#include <filesystem>
namespace chalet
{
namespace fs = std::filesystem;
}
#endif
