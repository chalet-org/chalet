/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_VCXPROJ_GEN_HPP
#define CHALET_VS_VCXPROJ_GEN_HPP

#include "State/MSVCWarningLevel.hpp"
#include "Utility/Uuid.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CentralState;
class BuildState;
struct SourceTarget;
struct ProjectAdapterVCXProj;

struct VSVCXProjGen
{
	explicit VSVCXProjGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids);

	bool saveProjectFiles(const BuildState& inState, const SourceTarget& inProject);

private:
	bool saveProjectFile(const BuildState& inState, const SourceTarget& inProject, const std::string& inFilename);
	bool saveFiltersFile(const BuildState& inState, const SourceTarget& inProject, const std::string& inFilename);
	bool saveUserFile(const std::string& inFilename);

	const SourceTarget* getProjectFromStateContext(const BuildState& inState, const std::string& inName) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_cwd;
	const std::string& m_projectTypeGuid;
	const OrderedDictionary<Uuid>& m_targetGuids;

	std::string m_currentTarget;
	std::string m_currentGuid;
};
}

#endif // CHALET_VS_VCXPROJ_GEN_HPP
