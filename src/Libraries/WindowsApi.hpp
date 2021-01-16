/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_API_HPP
#define CHALET_WINDOWS_API_HPP

// Windows
#ifdef CHALET_WIN32
	#ifndef UNICODE
		#define UNICODE
	#endif

	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#ifndef WIN32_ICON_MAIN
		#define WIN32_ICON_MAIN 1
	#endif

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#endif // CHALET_WINDOWS_API_HPP
