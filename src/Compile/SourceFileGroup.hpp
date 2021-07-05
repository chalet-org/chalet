/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_FILE_GROUP_HPP
#define CHALET_SOURCE_FILE_GROUP_HPP

namespace chalet
{
struct SourceFileGroup
{
	std::string sourceFile;
	std::string targetFile;
	std::string dependencyFile;
};
}

#endif // CHALET_SOURCE_FILE_GROUP_HPP
