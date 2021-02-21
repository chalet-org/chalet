/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr std::string_view Unicode::dot()
{
	return u8"\xE2\xAC\xA4";
}

/*****************************************************************************/
constexpr std::string_view Unicode::diamond()
{
	return u8"\xE2\x99\xA6";
}

/*****************************************************************************/
constexpr std::string_view Unicode::heavyCheckmark()
{
	return u8"\xE2\x9C\x94";
}

/*****************************************************************************/
constexpr std::string_view Unicode::heavyBallotX()
{
	return u8"\xE2\x9C\x98";
}

constexpr std::string_view Unicode::boldSaltire()
{
	return u8"\xF0\x9F\x9E\xAB";
}

/*****************************************************************************/
constexpr std::string_view Unicode::circledSaltire()
{
	return u8"\xE2\xAD\x99";
}

/*****************************************************************************/
constexpr std::string_view Unicode::heavyCurvedDownRightArrow()
{
	return u8"\xE2\x9E\xA5";
}

/*****************************************************************************/
constexpr std::string_view Unicode::heavyCurvedUpRightArrow()
{
	return u8"\xE2\x9E\xA6";
}

}
