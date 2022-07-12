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
	explicit VSVCXProjGen(const BuildState& inState, const std::string& inCwd, const std::string& inProjectTypeGuid, const OrderedDictionary<std::string>& inTargetGuids);

	bool saveToFile(const std::string& inTargetName);

private:
	bool saveFiltersFile(const std::string& inFilename);
	bool saveUserFile(const std::string& inFilename);

	StringList getSourceExtensions() const;
	StringList getHeaderExtensions() const;
	StringList getResourceExtensions() const;

	const BuildState& m_state;
	const std::string& m_cwd;
	const std::string& m_projectTypeGuid;
	const OrderedDictionary<std::string>& m_targetGuids;
};
}

#endif // CHALET_VS_VCXPROJ_GEN_HPP
