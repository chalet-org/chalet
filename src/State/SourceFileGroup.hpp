/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CodeLanguage.hpp"
#include "State/SourceDataType.hpp"
#include "State/SourceType.hpp"

namespace chalet
{
struct SourceFileGroup
{
	std::string sourceFile;
	std::string objectFile;
	std::string dependencyFile;
	std::string otherFile;
	SourceType type = SourceType::Unknown;
	SourceDataType dataType = SourceDataType::None;
};

using SourceFileGroupList = std::vector<Unique<SourceFileGroup>>;
}
