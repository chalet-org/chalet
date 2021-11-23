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
struct IBuildTarget;
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

	bool copyRunDependencies(const IBuildTarget& inProject);
	StringList getResolvedRunDependenciesList(const IBuildTarget& inProject);
	bool doSubChaletClean(const SubChaletTarget& inTarget);
	bool doCMakeClean(const CMakeTarget& inTarget);
	bool doLazyClean(const std::function<void()>& onClean = nullptr);

	bool addProjectToBuild(const SourceTarget& inProject);

	bool saveCompileCommands() const;

	// commands
	bool cmdBuild(const SourceTarget& inProject);
	bool cmdRebuild(const SourceTarget& inProject);
	bool cmdRun(const IBuildTarget& inTarget);
	bool cmdClean();

	bool runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand);
	bool runSubChaletTarget(const SubChaletTarget& inTarget);
	bool runCMakeTarget(const CMakeTarget& inTarget);
	std::string getRunTarget();
	bool createAppBundle();
	bool runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	BuildRouteList m_buildRoutes;

	CompileStrategy m_strategy;
	Unique<AssemblyDumper> m_asmDumper;

	Timer m_timer;

	// SourceTarget* m_project = nullptr;

	std::string m_runTargetName;

	bool m_directoriesMade = false;
};
}

#endif // CHALET_MAKEFILE_RUNNER_HPP
