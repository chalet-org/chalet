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
#include "Compile/CompilerTools.hpp"
#include "Core/CpuArchitecture.hpp"
#include "State/BuildCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/CommandLineInputs.hpp"
#include "Terminal/MsvcEnvironment.hpp"

namespace chalet
{
class BuildState
{
	const CommandLineInputs& m_inputs;
	std::string m_buildConfiguration;

public:
	explicit BuildState(const CommandLineInputs& inInputs);

	CacheTools tools;
	CompilerTools compilerTools;
	WorkspaceInfo info;
	BuildEnvironment environment;
	BuildPaths paths;
	MsvcEnvironment msvcEnvironment;
	ConfigurationOptions configuration;
	ProjectConfigurationList projects;
	DependencyList externalDependencies;
	AppBundle bundle;
	BuildCache cache;

	const std::string& buildConfiguration() const noexcept;
	void setBuildConfiguration(const std::string& inValue) noexcept;

	const std::string& platform() const noexcept;
	const StringList& notPlatforms() const noexcept;

	CpuArchitecture hostArchitecture() const noexcept;
	CpuArchitecture targetArchitecture() const noexcept;
	void setTargetArchitecture(const std::string& inValue) noexcept;

private:
	friend class Application;
	friend class Router;

	void initializeBuild();
	void initializeCache();

	CpuArchitecture m_targetArchitecture = CpuArchitecture::X64;
};
}

#endif // CHALET_BUILD_STATE_HPP
