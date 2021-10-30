/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_RUNNER_HPP
#define CHALET_MAKEFILE_RUNNER_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Router/Route.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
class BuildState;
struct SubChaletTarget;
struct CMakeTarget;
struct SourceTarget;
struct AssemblyDumper;
struct ScriptBuildTarget;
struct CommandLineInputs;

class BuildManager
{
	using BuildAction = std::function<bool(BuildManager&, const SourceTarget& inTarget)>;
	using BuildRouteList = std::unordered_map<Route, BuildAction>;

public:
	explicit BuildManager(const CommandLineInputs& inInputs, BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildManager);
	~BuildManager();

	bool run(const Route inRoute, const bool inShowSuccess = true);

private:
	void printBuildInformation();
	std::string getBuildStrategyName() const;

	bool copyRunDependencies(const SourceTarget& inProject);
	StringList getResolvedRunDependenciesList(const SourceTarget& inProject);
	bool doClean(const SourceTarget& inProject, const std::string& inTarget, const SourceFileGroupList& inGroups, const bool inFullClean = false);
	bool doSubChaletClean(const SubChaletTarget& inTarget);
	bool doCMakeClean(const CMakeTarget& inTarget);
	bool doLazyClean();

	bool addProjectToBuild(const SourceTarget& inProject, const Route inRoute);

	// commands
	bool cmdBuild(const SourceTarget& inProject);
	bool cmdRebuild(const SourceTarget& inProject);
	bool cmdRun(const SourceTarget& inProject);
	bool cmdClean();

	bool runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand);
	bool runSubChaletTarget(const SubChaletTarget& inTarget);
	bool runCMakeTarget(const CMakeTarget& inTarget);
	std::string getRunTarget();
	bool createAppBundle();
	bool runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable, const std::string& inOutputFolder);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	BuildRouteList m_buildRoutes;
	StringList m_removeCache;

	CompileStrategy m_strategy;
	Unique<AssemblyDumper> m_asmDumper;

	Timer m_timer;

	// SourceTarget* m_project = nullptr;

	std::string m_runTargetName;

	bool m_directoriesMade = false;
};
}

#endif // CHALET_MAKEFILE_RUNNER_HPP
