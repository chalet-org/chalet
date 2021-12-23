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
struct StatePrototype;
struct WorkspaceCache;
struct WorkspaceEnvironment;
struct ICompileEnvironment;
struct IBuildTarget;
struct ICompileEnvironment;

class BuildState
{
	struct Impl;
	Unique<Impl> m_impl;

public:
	explicit BuildState(CommandLineInputs inInputs, StatePrototype& inPrototype);
	CHALET_DISALLOW_COPY_MOVE(BuildState);
	~BuildState();

	AncillaryTools& tools;
	WorkspaceCache& cache;

	BuildInfo& info;
	WorkspaceEnvironment& workspace;
	CompilerTools& toolchain;
	BuildPaths& paths;
	BuildConfiguration& configuration;
	std::vector<Unique<IBuildTarget>>& targets;
	const CommandLineInputs& inputs;
	ICompileEnvironment* environment = nullptr;

	bool initialize();
	bool doBuild(const bool inShowSuccess = true);
	bool doBuild(const Route inRoute, const bool inShowSuccess = true);

	void makeLibraryPathVariables();

	void replaceVariablesInPath(std::string& outPath, const std::string& inName, const bool inBuildTarget = true) const;
	const std::string& uniqueId() const noexcept;

private:
	bool initializeBuildConfiguration();
	bool parseToolchainFromSettingsJson();
	bool parseBuildJson();

	bool initializeToolchain();
	bool initializeBuild();
	void initializeCache();

	bool validateState();
	bool validateSigningIdentity();

	bool makePathVariable();
	void makeCompilerDiagnosticsVariables();
	void enforceArchitectureInPath();
	void enforceArchitectureInPath(std::string& outPathVariable);

	std::string getUniqueIdForState() const;

	std::string m_uniqueId;
};
}

#endif // CHALET_BUILD_STATE_HPP
