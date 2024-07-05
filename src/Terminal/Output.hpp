/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Terminal/Color.hpp"
#include "Terminal/ColorTheme.hpp"

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

std::ostream& getErrStream();

bool getUserInput(
	const std::string& inUserQuery, std::string& outResult, std::string inNote = std::string(),
	const std::function<bool(std::string&)>& onValidate = [](std::string& input) {UNUSED(input);return true; }, const bool inFailOnFalse = true);
bool getUserInputYesNo(const std::string& inUserQuery, const bool inDefaultYes, std::string inNote = std::string());

const std::string& getAnsiStyle(const Color inColor);
std::string getAnsiStyleRaw(const Color inColor);

void displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage);
void lineBreak(const bool inForce = false);
void lineBreakStderr();
void previousLine(const bool inForce = false);
void print(const Color inColor, const std::string& inText);
void print(const Color inColor, const StringList& inList);
void printCommand(const std::string& inText);
void printCommand(const StringList& inList);
void printInfo(const std::string& inText);
void printFlair(const std::string& inText);
void printSeparator(const char inChar = '-');

void msgFetchingDependency(const std::string& inPath);
void msgRemovedUnusedDependency(const std::string& inDependencyName);

void msgConfigureCompleted(const std::string& inWorkspaceName);
void msgBuildSuccess();
void msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName);
void msgCommandPoolError(const std::string& inMessage);
void msgBuildFail();
void msgProfilerStartedGprof(const std::string& inProfileAnalysis);
void msgProfilerStartedSample(const std::string& inExecutable, const u32 inDuration, const u32 inSamplingInterval);
void msgProfilerDone(const std::string& inProfileAnalysis);
void msgProfilerDoneAndLaunching(const std::string& inProfileAnalysis, const std::string& inApplication);
void msgClean(const std::string& inBuildConfiguration);
void msgTargetOfType(const std::string& inLabel, const std::string& inName, const Color inColor);
void msgTargetDescription(const std::string& inDescription, const Color inColor);
void msgRun(const std::string& inName);
void msgScanningForModuleDependencies();
void msgBuildingRequiredHeaderUnits();
void msgModulesCompiling();

// void msgRemoving(const std::string& inPath);
void msgCopying(const std::string& inFrom, const std::string& inTo);
void msgAction(const std::string& inLabel, const std::string& inTo);
void msgActionEllipsis(const std::string& inLabel, const std::string& inTo);
}
}
