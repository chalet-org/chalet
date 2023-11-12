/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Utility/Uuid.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSSolutionGen
{
	explicit VSSolutionGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inProjectTypeGuid, const OrderedDictionary<Uuid>& inTargetGuids);

	bool saveToFile(const std::string& inFilename);

private:
	bool projectWillBuild(const std::string& inName, const std::string& inConfigName) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_projectTypeGuid;
	const OrderedDictionary<Uuid>& m_targetGuids;
};
}
