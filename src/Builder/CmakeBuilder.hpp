/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
	std::string getCacheFile() const;

	StringList getGeneratorCommand();
	StringList getBuildCommand() const;
	StringList getBuildCommand(const std::string& inOutputLocation) const;
	StringList getInstallCommand() const;
	StringList getInstallCommand(const std::string& inOutputLocation) const;

private:
	bool dependencyHasUpdated() const;
	StringList getGeneratorCommand(const std::string& inLocation, const std::string& inBuildFile) const;

	std::string getLocation() const;

	std::string getGenerator() const;
	std::string getArchitecture() const;
	void addCmakeDefines(StringList& outList) const;
	std::string getCMakeCompatibleBuildConfiguration() const;
	std::string getCmakeSystemName(const std::string& inTargetTriple) const;

	std::string getQuotedPath(const std::string& inPath) const;

	const std::string& outputLocation() const;

	bool usesNinja() const;

	const BuildState& m_state;
	const CMakeTarget& m_target;

	u32 m_cmakeVersionMajorMinor = 0;

	bool m_quotedPaths = false;
};
}
