/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DEFINES_PLATFORM_HPP
#define CHALET_DEFINES_PLATFORM_HPP

#if defined(_DEBUG) || defined(DEBUG)
	#ifndef CHALET_DEBUG
		#define CHALET_DEBUG
	#endif
#endif

#if defined(__APPLE__)
	#ifndef CHALET_MACOS
		#define CHALET_MACOS
	#endif
#elif defined(__linux__)
	#ifndef CHALET_LINUX
		#define CHALET_LINUX
	#endif
#elif defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
	#ifndef CHALET_WIN32
		#define CHALET_WIN32
	#endif
#else
	#error "Unknown platform";
#endif

#endif // CHALET_DEFINES_PLATFORM_HPP