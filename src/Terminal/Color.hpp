/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COLOR_HPP
#define CHALET_COLOR_HPP

namespace chalet
{
enum class Color : uchar
{
	Reset = 0,
	//
	Black = 30,
	Red = 31,
	Green = 32,
	Yellow = 33,
	Blue = 34,
	Magenta = 35,
	Cyan = 36,
	White = 37,
	//
	BrightBlack = 90,
	BrightRed = 91,
	BrightGreen = 92,
	BrightYellow = 93,
	BrightBlue = 94,
	BrightMagenta = 95,
	BrightCyan = 96,
	BrightWhite = 97,
	//
	BrightBlackBold = 130,
	BrightRedBold = 131,
	BrightGreenBold = 132,
	BrightYellowBold = 133,
	BrightBlueBold = 134,
	BrightMagentaBold = 135,
	BrightCyanBold = 136,
	BrightWhiteBold = 137,

};
}

#endif // CHALET_COLOR_HPP
