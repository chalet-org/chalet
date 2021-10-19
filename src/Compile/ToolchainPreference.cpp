/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/ToolchainPreference.hpp"

namespace chalet
{
/*****************************************************************************/
void ToolchainPreference::setType(const ToolchainType inType)
{
	if (type == ToolchainType::Unknown)
		type = inType;
}
}
