/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
