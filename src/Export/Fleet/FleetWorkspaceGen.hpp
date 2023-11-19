/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Libraries/Json.hpp"

namespace chalet
{
class BuildState;

struct FleetWorkspaceGen
{
	explicit FleetWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, const std::string& inAllBuildName);

	bool saveToPath(const std::string& inPath);

private:
	struct RunConfiguration
	{
		std::string name;
		std::string config;
		std::string arch;
		// std::string outputFile;
		// std::string args;
		// std::map<std::string, std::string> env;
	};
	bool createRunJsonFile(const std::string& inFilename);

	BuildState& getDebugState() const;

	const std::string& getToolchain() const;
	StringList getArchitectures(const std::string& inToolchain) const;

	Json makeRunConfiguration(const RunConfiguration& inRunConfig) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;
	const std::string& m_allBuildName;

	std::vector<RunConfiguration> m_runConfigs;
	StringList m_arches;

	std::string m_toolchain;
	std::string m_cwd;
};
}
