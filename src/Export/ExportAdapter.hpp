/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;

struct RunConfiguration
{
	std::string name;
	std::string config;
	std::string arch;
	std::string outputFile;
	StringList args;
	std::map<std::string, std::string> env;
};

using RunConfigurationList = std::vector<RunConfiguration>;

struct ExportAdapter
{
	ExportAdapter(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, std::string inAllBuildName);

	bool initialize();

	bool createCompileCommandsStub() const;

	const StringList& arches() const noexcept;
	const std::string& toolchain() const noexcept;
	const std::string& cwd() const noexcept;
	const std::string& debugConfiguration() const noexcept;
	const std::string& allBuildName() const noexcept;

	std::string getDefaultTargetName() const;
	std::string getAllTargetName() const;
	std::string getRunConfigLabel(const RunConfiguration& inRunConfig) const;
	std::string getLabelArchitecture(const RunConfiguration& inRunConfig) const;
	std::string getRunConfigExec() const;
	StringList getRunConfigArguments(const RunConfiguration& inRunConfig, std::string cmd = std::string(), const bool inWithRun = true) const;

	RunConfigurationList getBasicRunConfigs() const;
	RunConfigurationList getFullRunConfigs() const;

	std::string getPathVariableForState(const BuildState& inState) const;

	BuildState& getDebugState() const;
	BuildState* getStateFromRunConfig(const RunConfiguration& inRunConfig) const;

private:
	const std::string& getToolchain() const;
	StringList getArchitectures(const std::string& inToolchain) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;
	const std::string m_allBuildName;

	StringList m_arches;

	std::string m_toolchain;
	std::string m_cwd;
};
}
