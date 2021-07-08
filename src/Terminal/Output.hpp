/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OUTPUT_HPP
#define CHALET_OUTPUT_HPP

#include "Terminal/Color.hpp"
#include "Terminal/ColorTheme.hpp"

namespace chalet
{
namespace Output
{
void setTheme(const ColorTheme& inTheme);
const ColorTheme& theme();

bool quietNonBuild();
void setQuietNonBuild(const bool inValue);

bool cleanOutput();
bool showCommands();
void setShowCommands(const bool inValue);
void setShowCommandOverride(const bool inValue);

bool getUserInput(
	const std::string& inUserQuery, std::string& outResult, const Color inAnswerColor, std::string inNote = std::string(),
	const std::function<bool(std::string&)>& onValidate = [](std::string& input) {UNUSED(input);return true; });
bool getUserInputYesNo(const std::string& inUserQuery, const Color inAnswerColor, std::string inNote = std::string());

std::string getAnsiStyle(const Color inColor, const bool inBold = false);
std::string getAnsiStyle(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold = false);
std::string getAnsiStyleUnescaped(const Color inColor, const bool inBold = false);
std::string getAnsiStyleUnescaped(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold = false);
std::string getAnsiReset();

void displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage, const bool inBold = true);
void resetStdout();
void resetStderr();
void lineBreak();
void lineBreakStderr();
void previousLine();
void print(const Color inColor, const std::string& inText, const bool inBold = false);
void print(const Color inColor, const StringList& inList, const bool inBold = false);
void printCommand(const std::string& inText);
void printCommand(const StringList& inList);
void printInfo(const std::string& inText);
void printFlair(const std::string& inText);
void printSeparator(const char inChar = '-');

void msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgDisplayBlack(const std::string& inString);

void msgConfigureCompleted();
void msgBuildSuccess();
void msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName);
void msgBuildFail();
void msgCleaning();
void msgNothingToClean();
void msgCleaningRebuild();
void msgBuildProdError(const std::string& inBuildConfiguration);
void msgProfilerStartedGprof(const std::string& inProfileAnalysis);
void msgProfilerStartedSample(const std::string& inExecutable, const uint inDuration, const uint inSamplingInterval);
void msgProfilerDone(const std::string& inProfileAnalysis);
void msgProfilerDoneInstruments(const std::string& inProfileAnalysis);
void msgClean(const std::string& inBuildConfiguration);
void msgBuild(const std::string& inBuildConfiguration, const std::string& inName);
void msgRebuild(const std::string& inBuildConfiguration, const std::string& inName);
void msgScript(const std::string& inName, const Color inColor);
void msgScriptDescription(const std::string& inDescription, const Color inColor);
void msgRun(const std::string& inBuildConfiguration, const std::string& inName);
void msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName);
void msgProfile(const std::string& inBuildConfiguration, const std::string& inName);

// void msgRemoving(const std::string& inPath);
void msgCopying(const std::string& inFrom, const std::string& inTo);
}
}

#endif // CHALET_OUTPUT_HPP
