/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#ifndef FMT_HEADER_ONLY
	#define FMT_HEADER_ONLY
#endif

#if defined(CHALET_MSVC)
	#pragma warning(push)
	#pragma warning(disable : 4189)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wunused-parameter"
	#pragma GCC diagnostic ignored "-Wtype-limits"
	#pragma GCC diagnostic ignored "-Wshadow"
	#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"
#endif

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#define FMT_ARG(x) fmt::arg(#x, x)
