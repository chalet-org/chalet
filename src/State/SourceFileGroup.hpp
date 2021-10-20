/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_FILE_GROUP_HPP
#define CHALET_SOURCE_FILE_GROUP_HPP

#include "State/SourceType.hpp"

namespace chalet
{
struct SourceFileGroup
{
	std::string sourceFile;
	std::string objectFile;
	std::string dependencyFile;
	std::string assemblyFile;
	SourceType type = SourceType::Unknown;
};

using SourceFileGroupList = std::vector<Unique<SourceFileGroup>>;
}

#endif // CHALET_SOURCE_FILE_GROUP_HPP
