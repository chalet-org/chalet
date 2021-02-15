/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_STACK_TRACE_HPP
#define CHALET_STACK_TRACE_HPP

#include "Libraries/WindowsApi.hpp" // first

// #ifdef CHALET_WIN32
// 	#define _MSC_VER 1
// #endif

#ifdef CHALET_MSVC
	#pragma warning(push)
	#pragma warning(disable : 4456)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wunused-result"
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#include <ust/ust.hpp>

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

// #ifdef CHALET_WIN32
// 	#undef _MSC_VER
// #endif

#endif // CHALET_STACK_TRACE_HPP
