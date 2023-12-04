/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Router/CommandRoute.hpp"

namespace chalet
{
struct BuildConfiguration;
struct BuildInfo;
struct BuildPaths;
struct AncillaryTools;
struct CompilerTools;
struct CommandLineInputs;
struct CentralState;
struct WorkspaceCache;
struct WorkspaceEnvironment;
struct IBuildEnvironment;
struct IBuildTarget;
struct IExternalDependency;
struct IBuildEnvironment;
struct IDistTarget;

class BuildState
{
	struct Impl;
	Unique<Impl> m_impl;

public:
	explicit BuildState(CommandLineInputs inInputs, CentralState& inCentralState);
	CHALET_DISALLOW_COPY_MOVE(BuildState);
	~BuildState();

	bool initialize();
	bool generateProjects();
	bool doBuild(const CommandRoute& inRoute, const bool inShowSuccess = true);
	void setCacheEnabled(const bool inValue);

	void makeLibraryPathVariables();

	bool replaceVariablesInString(std::string& outString, const IBuildTarget* inTarget, const bool inCheckHome = true, const std::function<std::string(std::string)>& onFail = nullptr) const;
	bool replaceVariablesInString(std::string& outString, const IDistTarget* inTarget, const bool inCheckHome = true, const std::function<std::string(std::string)>& onFail = nullptr) const;
	const std::string& cachePathId() const noexcept;

	void getTargetDependencies(StringList& outList, const std::string& inTargetName, const bool inWithSelf) const;

	bool isSubChaletTarget() const noexcept;

	CentralState& getCentralState();
	const CentralState& getCentralState() const;
	const IBuildTarget* getFirstValidRunTarget() const;

	AncillaryTools& tools;
	WorkspaceCache& cache;

	BuildInfo& info;
	WorkspaceEnvironment& workspace;
	CompilerTools& toolchain;
	BuildPaths& paths;
	BuildConfiguration& configuration;
	std::vector<Unique<IBuildTarget>>& targets;
	std::vector<Unique<IDistTarget>>& distribution;
	const CommandLineInputs& inputs;
	const std::vector<Unique<IExternalDependency>>& externalDependencies;
	IBuildEnvironment* environment = nullptr;

private:
	bool initializeBuildConfiguration();
	bool parseToolchainFromSettingsJson();
	bool parseChaletJson();

	bool initializeToolchain();
	bool initializeBuild();
	void initializeCache();

	bool validateState();
	bool validateDistribution();

	void makePathVariable();
	void makeCompilerDiagnosticsVariables();
	void enforceArchitectureInPath();
	void enforceArchitectureInPath(std::string& outPathVariable);

	void generateUniqueIdForState();

	std::string m_cachePathId;

	bool m_cacheEnabled = true;
	bool m_isSubChaletTarget = false;
};
}
