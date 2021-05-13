/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CMAKE_BUILDER_HPP
#define CHALET_CMAKE_BUILDER_HPP

#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class CmakeBuilder
{
public:
	explicit CmakeBuilder(const BuildState& inState, const ProjectConfiguration& inProject, const bool inCleanOutput);

	bool run();

private:
	std::string getGenerator() const;
	StringList getGeneratorCommand(const std::string& inLocation) const;
	StringList getBuildCommand(const std::string& inLocation) const;

	const BuildState& m_state;
	const ProjectConfiguration& m_project;

	const bool m_cleanOutput;
};
}

#endif // CHALET_CMAKE_BUILDER_HPP
