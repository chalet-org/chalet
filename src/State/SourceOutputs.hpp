/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SOURCE_OUTPUTS_HPP
#define CHALET_SOURCE_OUTPUTS_HPP

namespace chalet
{
struct SourceOutputs
{
	StringList objectList;
	StringList dependencyList;
	StringList assemblyList;

	std::string target;
	std::string pch;

	StringList directories;
	StringList fileExtensions;
};
}

#endif // CHALET_SOURCE_OUTPUTS_HPP
