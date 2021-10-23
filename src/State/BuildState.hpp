/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/CompilerTools.hpp"
#include "State/WorkspaceEnvironment.hpp"

#include "State/Dependency/IBuildDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct StatePrototype;
struct AncillaryTools;
struct CompilerConfig;
struct WorkspaceCache;
struct ICompileEnvironment;

class BuildState
{
	const CommandLineInputs m_inputs;
	StatePrototype& m_prototype;

public:
	explicit BuildState(CommandLineInputs inInputs, StatePrototype& inStatePrototype);
	CHALET_DISALLOW_COPY_MOVE(BuildState);
	~BuildState();

	const AncillaryTools& tools;
	const DistributionTargetList& distribution;
	WorkspaceCache& cache;
	BuildDependencyList& externalDependencies;

	BuildInfo info;
	WorkspaceEnvironment workspace;
	CompilerTools toolchain;
	BuildPaths paths;
	Unique<ICompileEnvironment> environment;
	BuildConfiguration configuration;
	BuildTargetList targets;

	bool initialize();
	bool initializeForConfigure();
	bool doBuild(const bool inShowSuccess = true);
	bool doBuild(const Route inRoute, const bool inShowSuccess = true);

	std::string getUniqueIdForState(const StringList& inOther) const;

	CompilerConfig& getCompilerConfig(const CodeLanguage inLanguage);
	const CompilerConfig& getCompilerConfig(const CodeLanguage inLanguage) const;

private:
	bool initializeBuildConfiguration();
	bool parseToolchainFromSettingsJson();
	bool parseBuildJson();

	bool initializeToolchain();
	bool initializeBuild();
	void initializeCache();
	bool validateState();

	bool makePathVariable();
	void makeCompilerDiagnosticsVariables();
	void makeLibraryPathVariables();
	void enforceArchitectureInPath();
	void enforceArchitectureInPath(std::string& outPathVariable);

	std::unordered_map<CodeLanguage, Unique<CompilerConfig>> m_configs;
};
}

#endif // CHALET_BUILD_STATE_HPP
