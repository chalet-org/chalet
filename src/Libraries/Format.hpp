/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_FORMAT_HPP
#define CHALET_FORMAT_HPP

#ifndef FMT_HEADER_ONLY
	#define FMT_HEADER_ONLY
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wnon-virtual-dtor"

#include <fmt/color.h>
#include <fmt/core.h>
#include <fmt/format-inl.h>
#include <fmt/format.h>

#pragma GCC diagnostic pop

#define FMT_ARG(x) fmt::arg(#x, x)

#endif // CHALET_FORMAT_HPP
