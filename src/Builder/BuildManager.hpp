/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_RUNNER_HPP
#define CHALET_MAKEFILE_RUNNER_HPP

#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Router/Route.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "State/Target/ScriptTarget.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
class BuildManager
{
	using BuildAction = std::function<bool(BuildManager&, const ProjectTarget& inTarget)>;
	using BuildRouteList = std::unordered_map<Route, BuildAction>;

public:
	explicit BuildManager(const CommandLineInputs& inInputs, BuildState& inState);

	bool run(const Route inRoute);

private:
	void printBuildInformation();
	std::string getBuildStrategyName() const;

	bool copyRunDependencies(const ProjectTarget& inProject);
	StringList getResolvedRunDependenciesList(const StringList& inRunDependencies, const CompilerConfig& inConfig);
	bool doRun(const ProjectTarget& inProject);
	bool doClean(const ProjectTarget& inProject, const std::string& inTarget, const StringList& inObjectList, const StringList& inDepList, const bool inFullClean = false);
	bool doLazyClean();

	bool cacheRecipe(const ProjectTarget& inProject, const Route inRoute);

	// commands
	bool cmdBuild(const ProjectTarget& inProject);
	bool cmdRebuild(const ProjectTarget& inProject);
	bool cmdRun(const ProjectTarget& inProject);
	bool cmdClean();
	// bool cmdProfile();

	bool runScriptTarget(const ScriptTarget& inScript);
	bool runCMakeTarget(const CMakeTarget& inTarget);
	std::string getRunProject();
	bool createAppBundle();
	bool runProfiler(const ProjectTarget& inProject, const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);

	void testTerminalMessages();

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	BuildRouteList m_buildRoutes;
	StringList m_removeCache;

	CompileStrategy m_strategy;

	// ProjectTarget* m_project = nullptr;

	std::string m_runProjectName;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_MAKEFILE_RUNNER_HPP
