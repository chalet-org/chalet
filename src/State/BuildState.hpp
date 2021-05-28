/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUILD_STATE_HPP
#define CHALET_BUILD_STATE_HPP

#include "BuildJson/AppBundle.hpp"
#include "BuildJson/BuildEnvironment.hpp"
#include "BuildJson/ConfigurationOptions.hpp"
#include "BuildJson/DependencyGit.hpp"
#include "BuildJson/Target/IBuildTarget.hpp"
#include "BuildJson/WorkspaceInfo.hpp"
#include "CacheJson/CacheTools.hpp"
#include "Compile/CompilerTools.hpp"
#include "State/BuildCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/CommandLineInputs.hpp"
#include "Terminal/MsvcEnvironment.hpp"

namespace chalet
{
class BuildState
{
	const CommandLineInputs& m_inputs;

public:
	explicit BuildState(const CommandLineInputs& inInputs);

	CacheTools tools;
	CompilerTools compilerTools;
	WorkspaceInfo info;
	BuildEnvironment environment;
	BuildPaths paths;
	MsvcEnvironment msvcEnvironment;
	ConfigurationOptions configuration;
	BuildTargetList targets;
	DependencyList externalDependencies;
	AppBundle bundle;
	BuildCache cache;

private:
	friend class Application;
	friend class Router;

	void initializeBuild();
	void initializeCache();
};
}

#endif // CHALET_BUILD_STATE_HPP
