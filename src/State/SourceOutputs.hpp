/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_OUTPUTS_HPP
#define CHALET_SOURCE_OUTPUTS_HPP

#include "State/SourceFileGroup.hpp"

namespace chalet
{
struct SourceOutputs
{
	SourceFileGroupList groups;

	std::string target;

	StringList objectListLinker;
	StringList directories;
};
}

#endif // CHALET_SOURCE_OUTPUTS_HPP
