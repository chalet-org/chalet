/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OUTPUT_HPP
#define CHALET_OUTPUT_HPP

#include "Terminal/Color.hpp"
#include "Terminal/ColorTheme.hpp"
#include "Terminal/Formatting.hpp"

namespace chalet
{
namespace Output
{
#if defined(CHALET_WIN32)
bool ansiColorsSupportedInComSpec();
#endif

void setTheme(const ColorTheme& inTheme);
const ColorTheme& theme();

bool quietNonBuild();
void setQuietNonBuild(const bool inValue);

bool cleanOutput();
bool showCommands();
void setShowCommands(const bool inValue);
void setShowCommandOverride(const bool inValue);

bool showBenchmarks();
void setShowBenchmarks(const bool inValue);

bool getUserInput(
	const std::string& inUserQuery, std::string& outResult, std::string inNote = std::string(),
	const std::function<bool(std::string&)>& onValidate = [](std::string& input) {UNUSED(input);return true; });
bool getUserInputYesNo(const std::string& inUserQuery, const bool inDefaultYes, std::string inNote = std::string());

std::string getAnsiStyle(const Color inColor);
std::string getAnsiStyleRaw(const Color inColor);
std::string getAnsiStyleForMakefile(const Color inColor);
std::string getAnsiStyleForceFormatting(const Color inColor, const Formatting inFormatting);

void displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage);
void resetStdout();
void resetStderr();
void lineBreak();
void lineBreakStderr();
void previousLine();
void print(const Color inColor, const std::string& inText);
void print(const Color inColor, const StringList& inList);
void printCommand(const std::string& inText);
void printCommand(const StringList& inList);
void printInfo(const std::string& inText);
void printFlair(const std::string& inText);
void printSeparator(const char inChar = '-');

void msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgRemovedUnusedDependency(const std::string& inDependencyName);

void msgConfigureCompleted(const std::string& inWorkspaceName);
void msgBuildSuccess();
void msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName);
void msgCommandPoolError(const std::string& inMessage);
void msgBuildFail();
void msgCleaning();
void msgNothingToClean();
void msgProfilerStartedGprof(const std::string& inProfileAnalysis);
void msgProfilerStartedSample(const std::string& inExecutable, const uint inDuration, const uint inSamplingInterval);
void msgProfilerDone(const std::string& inProfileAnalysis);
void msgProfilerDoneInstruments(const std::string& inProfileAnalysis);
void msgClean(const std::string& inBuildConfiguration);
void msgBuild(const std::string& inName);
void msgRebuild(const std::string& inName);
void msgDistribution(const std::string& inName);
void msgScript(const std::string& inName, const Color inColor);
void msgTargetDescription(const std::string& inDescription, const Color inColor);
void msgRun(const std::string& inName);
void msgScanningForModuleDependencies();
void msgBuildingRequiredHeaderUnits();
void msgModulesCompiling();

// void msgRemoving(const std::string& inPath);
void msgCopying(const std::string& inFrom, const std::string& inTo);
}
}

#endif // CHALET_OUTPUT_HPP
