/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Compile/Strategy/StrategyType.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "State/BuildCache.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/IBuildDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/SourceFileCache.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/WorkspaceInfo.hpp"
#include "Terminal/MsvcEnvironment.hpp"

namespace chalet
{
struct StatePrototype;
struct CacheTools;
struct BuildCache;

class BuildState
{
	const CommandLineInputs m_inputs;
	StatePrototype& m_prototype;

public:
	explicit BuildState(CommandLineInputs inInputs, StatePrototype& inJsonPrototype);

	const CacheTools& tools;
	const DistributionTargetList& distribution;
	BuildCache& cache;

	WorkspaceInfo info;
	CompilerTools compilerTools;
	BuildPaths paths;
	MsvcEnvironment msvcEnvironment;
	BuildConfiguration configuration;
	BuildTargetList targets;
	BuildDependencyList externalDependencies;
	SourceFileCache sourceCache;

	bool initialize(const bool inInstallDependencies);
	bool doBuild();
	bool doBuild(const Route inRoute);
	void saveCaches();

	const StringList& environmentPath() const noexcept;
	void addEnvironmentPaths(StringList&& inList);

	bool dumpAssembly() const noexcept;
	bool showCommands() const noexcept;
	uint maxJobs() const noexcept;

private:
	bool initializeBuildConfiguration();
	bool parseCacheJson();
	bool parseBuildJson();
	bool installDependencies();

	bool initializeBuild();
	void initializeCache();
	bool validateState();

	void enforceArchitectureInPath();
};
}

#endif // CHALET_BUILD_STATE_HPP
