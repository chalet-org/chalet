/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_VCXPROJ_GEN_HPP
#define CHALET_VS_VCXPROJ_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSVCXProjGen
{
	explicit VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd);

	bool saveToFile(const std::string& inTargetName);

private:
	bool saveFiltersFile(const std::string& inFilename);
	bool saveUserFile(const std::string& inFilename);

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_cwd;
};
}

#endif // CHALET_VS_VCXPROJ_GEN_HPP
