/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CMAKE_BUILDER_HPP
#define CHALET_CMAKE_BUILDER_HPP

#include "BuildJson/Target/CMakeTarget.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class CmakeBuilder
{
public:
	explicit CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget, const bool inCleanOutput);

	bool run();

private:
	std::string getGenerator() const;
	std::string getArch() const;
	StringList getGeneratorCommand(const std::string& inLocation) const;
	StringList getBuildCommand(const std::string& inLocation) const;

	const BuildState& m_state;
	const CMakeTarget& m_target;

	std::string m_outputLocation;

	const bool m_cleanOutput;
};
}

#endif // CHALET_CMAKE_BUILDER_HPP
