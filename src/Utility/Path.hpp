/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
namespace Path
{
void unix(std::string& outValue, const bool inRemoveNewLine = false);
void windows(std::string& outValue, const bool inRemoveNewLine = false);
}
}
