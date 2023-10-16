/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Router/CommandRoute.hpp"

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
struct ICompileEnvironment;
struct IBuildTarget;
struct IExternalDependency;
struct ICompileEnvironment;
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
	ICompileEnvironment* environment = nullptr;

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
	std::string m_uniqueId;

	bool m_cacheEnabled = true;
};
}

#endif // CHALET_BUILD_STATE_HPP
