/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Router/Route.hpp"

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
struct IBuildDependency;
struct ICompileEnvironment;

class BuildState
{
	struct Impl;
	Unique<Impl> m_impl;

public:
	explicit BuildState(CommandLineInputs inInputs, CentralState& inCentralState);
	CHALET_DISALLOW_COPY_MOVE(BuildState);
	~BuildState();

	bool initialize();
	bool doBuild(const bool inShowSuccess = true);
	bool doBuild(const Route inRoute, const bool inShowSuccess = true);

	void makeLibraryPathVariables();

	void replaceVariablesInPath(std::string& outPath, const std::string& inName, const bool isDefine = false) const;
	const std::string& uniqueId() const noexcept;

	AncillaryTools& tools;
	WorkspaceCache& cache;

	BuildInfo& info;
	WorkspaceEnvironment& workspace;
	CompilerTools& toolchain;
	BuildPaths& paths;
	BuildConfiguration& configuration;
	std::vector<Unique<IBuildTarget>>& targets;
	const CommandLineInputs& inputs;
	const std::vector<Unique<IBuildDependency>>& externalDependencies;
	ICompileEnvironment* environment = nullptr;

private:
	bool initializeBuildConfiguration();
	bool parseToolchainFromSettingsJson();
	bool parseChaletJson();

	bool initializeToolchain();
	bool initializeBuild();
	void initializeCache();

	bool validateState();

	bool makePathVariable();
	void makeCompilerDiagnosticsVariables();
	void enforceArchitectureInPath();
	void enforceArchitectureInPath(std::string& outPathVariable);

	std::string getUniqueIdForState() const;

	std::string m_uniqueId;
};
}

#endif // CHALET_BUILD_STATE_HPP
