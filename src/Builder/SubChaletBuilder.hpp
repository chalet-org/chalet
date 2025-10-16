/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SubChaletTarget;

class SubChaletBuilder
{
public:
	explicit SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const bool inQuotedPaths = false);

	std::string getBuildFile() const;

	bool run();

	void removeSettingsFile();

	StringList getBuildCommand(const bool inInstall = false, const bool hasSettings = true) const;
	StringList getBuildCommand(const std::string& inLocation, const std::string& inBuildFile, const bool inInstall = false, const bool hasSettings = true) const;

	StringList getInstallCommand(const bool hasSettings = true) const;

private:
	bool dependencyHasUpdated() const;
	std::string getLocation() const;
	std::string getQuotedPath(const std::string& inPath) const;

	const std::string& outputLocation() const;

	const BuildState& m_state;
	const SubChaletTarget& m_target;

	bool m_quotedPaths = false;
};
}
