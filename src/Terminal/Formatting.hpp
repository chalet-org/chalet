/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_FORMATTING_HPP
#define CHALET_FORMATTING_HPP

#include "Terminal/Color.hpp"

namespace chalet
{
enum class Formatting : std::underlying_type_t<Color>
{
	None = 0,
	Bold = 100,
	Dim = 200,
	Inverted = 700,
};
}

#endif // CHALET_FORMATTING_HPP
