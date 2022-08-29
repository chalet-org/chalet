/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_RUNNER_HPP
#define CHALET_MAKEFILE_RUNNER_HPP

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Router/CommandRoute.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;
struct SubChaletTarget;
struct CMakeTarget;
struct SourceTarget;
struct AssemblyDumper;
struct ProcessBuildTarget;
struct ScriptBuildTarget;

class BuildManager
{
	using BuildAction = std::function<bool(BuildManager&, const SourceTarget& inTarget)>;
	using BuildRouteList = std::unordered_map<RouteType, BuildAction>;

public:
	explicit BuildManager(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(BuildManager);
	~BuildManager();

	bool run(const CommandRoute& inRoute, const bool inShowSuccess = true);

private:
	void printBuildInformation();
	std::string getBuildStrategyName() const;

	bool copyRunDependencies(const IBuildTarget& inProject, uint& outCopied);
	bool doSubChaletClean(const SubChaletTarget& inTarget);
	bool doCMakeClean(const CMakeTarget& inTarget);
	bool doLazyClean(const std::function<void()>& onClean = nullptr, const bool inCleanExternals = false);

	bool addProjectToBuild(const SourceTarget& inProject);

	bool saveCompileCommands() const;
	bool onFinishBuild(const SourceTarget& inProject) const;

	// commands
	bool cmdBuild(const SourceTarget& inProject);
	bool cmdRebuild(const SourceTarget& inProject);
	bool cmdRun(const IBuildTarget& inTarget);
	bool cmdClean();

	bool runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand);
	bool runProcessTarget(const ProcessBuildTarget& inTarget);
	bool createAppBundle();
	bool runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable);
	bool runConfigureFileParser(const SourceTarget& inProject);
	bool runProcess(const StringList& inCmd, std::string outputFile, const bool inFromDist);
	bool runSubChaletTarget(const SubChaletTarget& inTarget);
	bool runCMakeTarget(const CMakeTarget& inTarget);
	bool runFullBuild();
	void stopTimerAndShowBenchmark(Timer& outTimer);

	BuildState& m_state;

	BuildRouteList m_buildRoutes;

	Dictionary<StringList> m_fileCache;

	CompileStrategy m_strategy;
	Unique<AssemblyDumper> m_asmDumper;

	Timer m_timer;

	// SourceTarget* m_project = nullptr;

	bool m_directoriesMade = false;
};
}

#endif // CHALET_MAKEFILE_RUNNER_HPP
