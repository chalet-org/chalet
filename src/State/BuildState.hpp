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
#include "BuildJson/ProjectConfiguration.hpp"
#include "BuildJson/WorkspaceInfo.hpp"
#include "CacheJson/CacheTools.hpp"
#include "Compile/CompilerCache.hpp"
#include "State/BuildCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
class BuildState
{
	const CommandLineInputs& m_inputs;
	std::string m_buildConfiguration;

public:
	explicit BuildState(const CommandLineInputs& inInputs);

	CacheTools tools;
	CompilerCache compilers;
	WorkspaceInfo info;
	BuildEnvironment environment;
	BuildPaths paths;
	ConfigurationOptions configuration;
	ProjectConfigurationList projects;
	DependencyList externalDependencies;
	AppBundle bundle;
	BuildCache cache;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

private:
	friend class Application;
	friend class Router;

	void initializeBuild();
	void initializeCache();
};
}

#endif // CHALET_BUILD_STATE_HPP
