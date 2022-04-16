/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_REGEX_HPP
#define CHALET_REGEX_HPP

#include <regex>

// #if !defined(CHALET_MSVC) && !(defined(CHALET_CLANG) && defined(CHALET_LINUX))
// #if !defined(CHALET_MSVC)
#if !defined(CHALET_REGEX_CTRE) && !defined(CHALET_REGEX_NO_CTRE)
	#define CHALET_REGEX_CTRE
	#if defined(CHALET_CLANG)
		#define CTRE_CXX_STANDARD 17
		#define CHALET_REGEX_CTRE_17
	#else
		#define CTRE_CXX_STANDARD 20
		#define CHALET_REGEX_CTRE_20
	#endif
	// #define CTRE_STRING_IS_UTF8 0
#endif
// #endif

#if defined(CHALET_MSVC)
	#pragma warning(push)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wpedantic"
	#pragma GCC diagnostic ignored "-Wtype-limits"
	#pragma GCC diagnostic ignored "-Wshadow"
#endif

#ifdef CHALET_REGEX_CTRE

	#include <ctre.hpp>

	/*template <auto& ptn>
	constexpr bool re()
	{
		#if CTRE_CNTTP_COMPILER_CHECK
		constexpr auto _ptn = ptn;
		#else
		constexpr auto& _ptn = ptn;
		#endif
		return ctll::parser<ctre::pcre, _ptn, ctre::pcre_actions>::template correct_with<ctre::pcre_context<>>;
	}*/
#endif

#if defined(CHALET_MSVC)
	#pragma warning(pop)
#else
	#pragma GCC diagnostic pop
#endif

#endif // CHALET_REGEX_HPP
