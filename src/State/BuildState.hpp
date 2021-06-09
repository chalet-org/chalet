/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "State/BuildCache.hpp"
#include "State/BuildEnvironment.hpp"
#include "State/BuildPaths.hpp"
#include "State/CacheTools.hpp"
#include "State/CompilerTools.hpp"
#include "State/ConfigurationOptions.hpp"
#include "State/Dependency/IBuildDependency.hpp"
#include "State/SourceFileCache.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/WorkspaceInfo.hpp"
#include "Terminal/MsvcEnvironment.hpp"

namespace chalet
{
class BuildState
{
	const CommandLineInputs& m_inputs;

public:
	explicit BuildState(const CommandLineInputs& inInputs);

	WorkspaceInfo info;
	CacheTools tools;
	CompilerTools compilerTools;
	BuildPaths paths;
	BuildEnvironment environment;
	MsvcEnvironment msvcEnvironment;
	ConfigurationOptions configuration;
	BuildTargetList targets;
	BuildDependencyList externalDependencies;
	DistributionTargetList distribution;
	BuildCache cache;
	SourceFileCache sourceCache;

	bool initialize(const bool inInstallDependencies);
	bool doBuild();
	bool doBuild(const Route inRoute);
	void saveCaches();

private:
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
