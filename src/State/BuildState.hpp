/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Cache/WorkspaceCache.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/IBuildDependency.hpp"
#include "State/Distribution/IDistTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/MsvcEnvironment.hpp"

namespace chalet
{
struct StatePrototype;
struct AncillaryTools;
struct WorkspaceCache;

class BuildState
{
	const CommandLineInputs m_inputs;
	StatePrototype& m_prototype;

public:
	explicit BuildState(CommandLineInputs inInputs, StatePrototype& inJsonPrototype);

	const AncillaryTools& tools;
	const DistributionTargetList& distribution;
	WorkspaceCache& cache;
	BuildDependencyList& externalDependencies;

	BuildInfo info;
	WorkspaceEnvironment environment;
	CompilerTools toolchain;
	BuildPaths paths;
	MsvcEnvironment msvcEnvironment;
	BuildConfiguration configuration;
	BuildTargetList targets;

	bool initialize();
	bool doBuild(const bool inShowSuccess = true);
	bool doBuild(const Route inRoute, const bool inShowSuccess = true);

private:
	bool initializeBuildConfiguration();
	bool parseToolchainFromSettingsJson();
	bool parseBuildJson();

	bool initializeBuild();
	void initializeCache();
	bool validateState();

	void enforceArchitectureInPath();
};
}

#endif // CHALET_BUILD_STATE_HPP
