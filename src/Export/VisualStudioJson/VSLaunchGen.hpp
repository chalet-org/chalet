/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_LAUNCH_GEN_HPP
#define CHALET_VS_LAUNCH_GEN_HPP

#include "Core/Arch.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct VSLaunchGen
{
	explicit VSLaunchGen(const std::vector<Unique<BuildState>>& inStates);

	bool saveToFile(const std::string& inFilename);

private:
	Json getConfiguration(const BuildState& inState, const IBuildTarget& inTarget) const;
	Json getEnvironment(const IBuildTarget& inTarget) const;

	const std::vector<Unique<BuildState>>& m_states;

	std::vector<const IBuildTarget*> m_executableTargets;
};
}

#endif // CHALET_VS_LAUNCH_GEN_HPP