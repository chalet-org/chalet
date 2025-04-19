/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

// Windows
#if defined(_WIN32) && !defined(RC_INVOKED)
	#ifndef UNICODE
		#define UNICODE
	#endif

	#ifndef _UNICODE
		#define _UNICODE
	#endif

	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <winuser.h>

	#include <tchar.h>

	#if defined(_MSC_VER)
		#pragma execution_character_set("utf-8")
	#endif
#endif
