/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_FILE_SYSTEM_HPP
#define CHALET_FILE_SYSTEM_HPP

#if defined(_MSC_VER) && _MSC_VER >= 1920
#elif defined(__cplusplus) && __cplusplus >= 201703L && defined(__has_include)
	#if __has_include(<filesystem>)
		// Note: std::filesystem is broken in GCC 8.x (possibly just in MinGW?)
		//  Use fallback in this instance just in case
		#if __clang_major__ >= 11 || __GNUC__ >= 9
		#else
			#define CHALET_FILE_SYSTEM_FALLBACK
			// #include <experimental/filesystem>
			// namespace fs = std::experimental::filesystem::v1;
		#endif
	#else
		#define CHALET_FILE_SYSTEM_FALLBACK
	#endif
#else
	#define CHALET_FILE_SYSTEM_FALLBACK
#endif
#ifdef CHALET_FILE_SYSTEM_FALLBACK
	#include <ghc/filesystem.hpp>
namespace chalet
{
namespace fs = ghc::filesystem;
}
	#undef CHALET_FILE_SYSTEM_FALLBACK
#else
	#include <filesystem>
namespace chalet
{
namespace fs = std::filesystem;
}
#endif

#endif // CHALET_FILE_SYSTEM_HPP
