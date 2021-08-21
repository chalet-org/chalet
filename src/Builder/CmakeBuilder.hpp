/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CMAKE_BUILDER_HPP
#define CHALET_CMAKE_BUILDER_HPP

namespace chalet
{
class BuildState;
struct CMakeTarget;

class CmakeBuilder
{
public:
	explicit CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget);

	bool run();

private:
	std::string getGenerator() const;
	std::string getPlatform() const;
	StringList getGeneratorCommand(const std::string& inLocation) const;
	void addCmakeDefines(StringList& outList) const;
	std::string getCMakeCompatibleBuildConfiguration() const;

	StringList getBuildCommand(const std::string& inLocation) const;

	const BuildState& m_state;
	const CMakeTarget& m_target;

	std::string m_outputLocation;
	std::string m_buildFile;
};
}

#endif // CHALET_CMAKE_BUILDER_HPP
