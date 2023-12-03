/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Core/Router/CommandRoute.hpp"
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
struct ValidationBuildTarget;

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
	void populateBuildTargets(const CommandRoute& inRoute);
	const IBuildTarget* getRunTarget(const CommandRoute& inRoute);

	void printBuildInformation();
	std::string getBuildStrategyName() const;

	bool copyRunDependencies(const IBuildTarget& inProject, u32& outCopied);
	bool doSubChaletClean(const SubChaletTarget& inTarget);
	bool doCMakeClean(const CMakeTarget& inTarget);
	bool doLazyClean(const bool inShowMessage, const bool inCleanExternals, const bool inForceCleanExternals);

	bool addProjectToBuild(const SourceTarget& inProject);

	bool onFinishBuild(const SourceTarget& inProject) const;

	bool checkIntermediateFiles() const;

	// commands
	bool cmdBuild(const SourceTarget& inProject);
	bool cmdRebuild(const SourceTarget& inProject);
	bool cmdRun(const IBuildTarget& inTarget);
	bool cmdClean();

	bool runScriptTarget(const ScriptBuildTarget& inScript, const bool inRunCommand);
	bool runProcessTarget(const ProcessBuildTarget& inTarget);
	bool runValidationTarget(const ValidationBuildTarget& inTarget);
	bool runProfiler(const SourceTarget& inProject, const StringList& inCommand, const std::string& inExecutable);
	bool runConfigureFileParser(const SourceTarget& inProject, const std::string& outFolder) const;
	bool runProcess(const StringList& inCmd, std::string outputFile, const bool inFromDist);
	bool runSubChaletTarget(const SubChaletTarget& inTarget);
	bool runCMakeTarget(const CMakeTarget& inTarget);
	bool runFullBuild();
	void stopTimerAndShowBenchmark(Timer& outTimer);

	void displayHeader(const std::string& inLabel, const IBuildTarget& inTarget, const std::string& inName = std::string()) const;

	BuildState& m_state;

	BuildRouteList m_buildRoutes;

	std::vector<IBuildTarget*> m_buildTargets;

	Dictionary<StringList> m_fileCache;

	CompileStrategy m_strategy;
	Unique<AssemblyDumper> m_asmDumper;

	Timer m_timer;

	// SourceTarget* m_project = nullptr;

	bool m_directoriesMade = false;
};
}
