/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UNICODE_HPP
#define CHALET_UNICODE_HPP

namespace chalet
{
namespace Unicode
{
constexpr std::string_view dot();
constexpr std::string_view diamond();
constexpr std::string_view heavyCheckmark();
constexpr std::string_view heavyBallotX();
constexpr std::string_view boldSaltire();
constexpr std::string_view circledSaltire();
constexpr std::string_view heavyCurvedDownRightArrow();
constexpr std::string_view heavyCurvedUpRightArrow();
}
}

#include "Terminal/Unicode.inl"

#endif // CHALET_UNICODE_HPP
