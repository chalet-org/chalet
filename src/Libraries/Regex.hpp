/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_REGEX_HPP
#define CHALET_REGEX_HPP

#ifdef CHALET_MSVC
	#pragma warning(push)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wtype-limits"
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#define CTRE_STRING_IS_UTF8 0

#include <ctre/ctre.hpp>

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#endif // CHALET_REGEX_HPP
