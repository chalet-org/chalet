/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class Color : u16
{
	Reset = 0,
	None = 1, // explicit none
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
	BlackBold = 130,
	RedBold = 131,
	GreenBold = 132,
	YellowBold = 133,
	BlueBold = 134,
	MagentaBold = 135,
	CyanBold = 136,
	WhiteBold = 137,
	//
	BrightBlackBold = 190,
	BrightRedBold = 191,
	BrightGreenBold = 192,
	BrightYellowBold = 193,
	BrightBlueBold = 194,
	BrightMagentaBold = 195,
	BrightCyanBold = 196,
	BrightWhiteBold = 197,
	//
	BlackDim = 230,
	RedDim = 231,
	GreenDim = 232,
	YellowDim = 233,
	BlueDim = 234,
	MagentaDim = 235,
	CyanDim = 236,
	WhiteDim = 237,
	//
	BrightBlackDim = 290,
	BrightRedDim = 291,
	BrightGreenDim = 292,
	BrightYellowDim = 293,
	BrightBlueDim = 294,
	BrightMagentaDim = 295,
	BrightCyanDim = 296,
	BrightWhiteDim = 297,
	//
	BlackInverted = 730,
	RedInverted = 731,
	GreenInverted = 732,
	YellowInverted = 733,
	BlueInverted = 734,
	MagentaInverted = 735,
	CyanInverted = 736,
	WhiteInverted = 737,
	//
	BrightBlackInverted = 790,
	BrightRedInverted = 791,
	BrightGreenInverted = 792,
	BrightYellowInverted = 793,
	BrightBlueInverted = 794,
	BrightMagentaInverted = 795,
	BrightCyanInverted = 796,
	BrightWhiteInverted = 797,
};
enum class Formatting : std::underlying_type_t<Color>
{
	None = 0,
	Bold = 100,
	Dim = 200,
	Inverted = 700,
};
}
