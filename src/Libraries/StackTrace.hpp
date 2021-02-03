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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"
#pragma GCC diagnostic ignored "-Wshadow"

#include <ust/ust.hpp>

#pragma GCC diagnostic pop

// #ifdef CHALET_WIN32
// 	#undef _MSC_VER
// #endif

#endif // CHALET_STACK_TRACE_HPP
