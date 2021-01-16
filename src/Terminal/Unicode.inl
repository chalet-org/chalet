/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr Constant Unicode::dot()
{
	return u8"\xE2\xAC\xA4";
}

/*****************************************************************************/
constexpr Constant Unicode::diamond()
{
	return u8"\xE2\xAF\x8C";
}

/*****************************************************************************/
constexpr Constant Unicode::heavyCheckmark()
{
	return u8"\xE2\x9C\x94";
}

/*****************************************************************************/
constexpr Constant Unicode::heavyBallotX()
{
	return u8"\xE2\x9C\x98";
}

constexpr Constant Unicode::boldSaltire()
{
	return u8"\xF0\x9F\x9E\xAB";
}

/*****************************************************************************/
constexpr Constant Unicode::circledSaltire()
{
	return u8"\xE2\xAD\x99";
}

/*****************************************************************************/
constexpr Constant Unicode::heavyCurvedDownRightArrow()
{
	return u8"\xE2\x9E\xA5";
}

/*****************************************************************************/
constexpr Constant Unicode::heavyCurvedUpRightArrow()
{
	return u8"\xE2\x9E\xA6";
}
}
