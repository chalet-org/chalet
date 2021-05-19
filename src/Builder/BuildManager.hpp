/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_RUNNER_HPP
#define CHALET_MAKEFILE_RUNNER_HPP

#include "BuildJson/ProjectConfiguration.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Router/Route.hpp"
#include "State/BuildState.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
class BuildManager
{
	using BuildAction = std::function<bool(BuildManager&)>;
	using BuildRouteList = std::unordered_map<Route, BuildAction>;

public:
	explicit BuildManager(const CommandLineInputs& inInputs, BuildState& inState);

	bool run(const Route inRoute);

private:
	bool doScript();
	bool copyRunDependencies(const ProjectConfiguration& inProject);
	StringList getResolvedRunDependenciesList(const StringList& inRunDependencies, const CompilerConfig& inConfig);
	bool doRun();
	bool doClean(const ProjectConfiguration& inProject, const std::string& inTarget, const StringList& inObjectList, const StringList& inDepList, const bool inFullClean = false);
	bool doLazyClean();

	bool cacheRecipe(const ProjectConfiguration& inProject, const Route inRoute);

	// commands
	bool cmdBuild();
	bool cmdRebuild();
	bool cmdRun();
	bool cmdClean();
	// bool cmdProfile();

	bool compileCMakeProject();
	std::string getRunProject();
	std::string getRunOutputFile();
	bool createAppBundle();
	bool runProfiler(const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);

	void testTerminalMessages();

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	BuildRouteList m_buildRoutes;
	StringList m_removeCache;

	CompileStrategy m_strategy;

	ProjectConfiguration* m_project = nullptr;

	std::string m_runProjectName;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_MAKEFILE_RUNNER_HPP
