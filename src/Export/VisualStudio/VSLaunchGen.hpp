/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_LAUNCH_GEN_HPP
#define CHALET_VS_LAUNCH_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct VSLaunchGen
{
	explicit VSLaunchGen(const BuildState& inState, const std::string& inCwd, const IBuildTarget& inTarget);

	bool saveToFile(const std::string& inFilename) const;

private:
	Json getConfiguration() const;
	Json getEnvironment() const;
	std::string getInheritEnvironment() const;

	const BuildState& m_state;
	const std::string& m_cwd;
	const IBuildTarget& m_target;
};
}

#endif // CHALET_VS_LAUNCH_GEN_HPP
