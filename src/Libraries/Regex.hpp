/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_REGEX_HPP
#define CHALET_REGEX_HPP

#include <regex>

#ifdef CHALET_MSVC
	#pragma warning(push)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wtype-limits"
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#define CTRE_STRING_IS_UTF8 0

#include <ctre/single-header/ctre.hpp>

#ifdef CHALET_MSVC
template <auto& ptn>
constexpr bool re()
{
	#if CTRE_CNTTP_COMPILER_CHECK
	constexpr auto _ptn = ptn;
	#else
	constexpr auto& _ptn = ptn;
	#endif
	return ctll::parser<ctre::pcre, _ptn, ctre::pcre_actions>::template correct_with<ctre::pcre_context<>>;
}
#endif

#ifdef CHALET_MSVC
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#endif // CHALET_REGEX_HPP
