/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "Core/CommandLineInputs.hpp"
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

private:
	friend class Application;
	friend class Router;

	bool initializeBuild();
	void initializeCache();
	bool validateState();
};
}

#endif // CHALET_BUILD_STATE_HPP
