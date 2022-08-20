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
	explicit CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget, const bool inQuotedPaths = false);

	bool run();

	std::string getBuildFile(const bool inForce = false) const;

	bool dependencyHasUpdate() const;
	StringList getGeneratorCommand();
	StringList getBuildCommand() const;
	StringList getBuildCommand(const std::string& inOutputLocation) const;

private:
	StringList getGeneratorCommand(const std::string& inLocation, const std::string& inBuildFile) const;

	std::string getLocation() const;
	std::string getOutputLocation() const;

	std::string getGenerator() const;
	std::string getArchitecture() const;
	void addCmakeDefines(StringList& outList) const;
	std::string getCMakeCompatibleBuildConfiguration() const;
	std::string getQuotedPath(const std::string& inPath) const;

	const BuildState& m_state;
	const CMakeTarget& m_target;

	std::string m_outputLocation;

	uint m_cmakeVersionMajorMinor = 0;

	bool m_quotedPaths = false;
};
}

#endif // CHALET_CMAKE_BUILDER_HPP
