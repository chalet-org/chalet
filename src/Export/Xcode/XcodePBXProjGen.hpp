/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_PBXPROJ_GEN_HPP
#define CHALET_XCODE_PBXPROJ_GEN_HPP

#include "Utility/Uuid.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct XcodePBXProjGen
{
	explicit XcodePBXProjGen(const std::vector<Unique<BuildState>>& inStates);

	bool saveToFile(const std::string& inFilename);

private:
	const std::vector<Unique<BuildState>>& m_states;
};
}

#endif // CHALET_XCODE_PBXPROJ_GEN_HPP
