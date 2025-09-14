/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportRunConfiguration.hpp"

namespace chalet
{
class BuildState;

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
	std::string getRunConfigLabel(const ExportRunConfiguration& inRunConfig) const;
	std::string getLabelArchitecture(const ExportRunConfiguration& inRunConfig) const;
	std::string getRunConfigExec() const;
	StringList getRunConfigArguments(const ExportRunConfiguration& inRunConfig, std::string cmd = std::string(), const bool inWithRun = true) const;

	ExportRunConfigurationList getBasicRunConfigs() const;
	ExportRunConfigurationList getFullRunConfigs() const;

	std::string getPathVariableForState(const BuildState& inState) const;

	BuildState& getDebugState() const;
	BuildState* getStateFromRunConfig(const ExportRunConfiguration& inRunConfig) const;

	void removeArchitectures(const StringList& inList);

private:
	const std::string& getToolchain() const;
	StringList getArchitectures(const std::string& inToolchain) const;

	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_debugConfiguration;
	const std::string m_allBuildName;

	StringList m_arches;
	mutable StringList m_invalidArches;

	std::string m_toolchain;
	std::string m_cwd;
};
}
