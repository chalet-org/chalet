/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Unicode.hpp"

namespace chalet
{
/*****************************************************************************/
// Looks funky on Mac & Linux with default fonts
inline const char* Unicode::dot()
{
	return u8"\u25BC"; // triangle
}

/*****************************************************************************/
inline const char* Unicode::degree()
{
	return u8"\u2218";
}

/*****************************************************************************/
inline const char* Unicode::triangle()
{
	return u8"\u25BC"; // triangle
}

/*****************************************************************************/
inline const char* Unicode::diamond()
{
	return u8"\u2666";
}

/*****************************************************************************/
inline const char* Unicode::heavyCheckmark()
{
	return u8"\u2714";
}

/*****************************************************************************/
inline const char* Unicode::heavyBallotX()
{
	return u8"\u2718";
}

inline const char* Unicode::boldSaltire()
{
	return u8"\xF0\x9F\x9E\xAB";
}

/*****************************************************************************/
inline const char* Unicode::circledSaltire()
{
	return u8"\u2B59";
}

/*****************************************************************************/
inline const char* Unicode::heavyCurvedDownRightArrow()
{
	return u8"\u27A5";
}

/*****************************************************************************/
inline const char* Unicode::heavyCurvedUpRightArrow()
{
	return u8"\u27A6";
}

inline const char* Unicode::rightwardsTripleArrow()
{
	return u8"\u21DB";
}

}
