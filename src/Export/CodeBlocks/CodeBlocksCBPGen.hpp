/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CODEBLOCKS_CBP_GEN_HPP
#define CHALET_CODEBLOCKS_CBP_GEN_HPP

#include "Utility/Uuid.hpp"
#include "XML/XmlFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct CodeBlocksCBPGen
{
	explicit CodeBlocksCBPGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inAllBuildName);

	bool saveToFile(const std::string& inFilename);

private:
	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_allBuildName;
};
}

#endif // CHALET_CODEBLOCKS_CBP_GEN_HPP
