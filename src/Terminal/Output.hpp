/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_OUTPUT_HPP
#define CHALET_OUTPUT_HPP

#include "Terminal/Color.hpp"

namespace chalet
{
namespace Output
{
std::string getAnsiStyle(const Color inColor, const bool inBold = false);
std::string getAnsiStyle(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold = false);
std::string_view getAnsiReset();

void displayStyledSymbol(const Color inColor, const std::string& inSymbol, const std::string& inMessage, const bool inBold = true);
void warnBlankKey(const std::string& inKey, const std::string& inDefault = "");
void warnBlankKeyInList(const std::string& inKey);
void resetStdout();
void resetStderr();
void lineBreak();
void print(const Color inColor, const std::string& inText);
void print(const Color inColor, const StringList& inList);

void msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag);
void msgDisplayBlack(const std::string& inString);

void msgBuildSuccess();
void msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName);
void msgLaunch(const std::string& inBuildDir, const std::string& inName);
void msgBuildFail();
void msgCleaning();
void msgNothingToClean();
void msgCleaningRebuild();
void msgBuildProdError(const std::string& inBuildConfiguration);
void msgProfilerStarted(const std::string& inProfileAnalysis);
void msgProfilerDone(const std::string& inProfileAnalysis);
void msgClean(const std::string& inBuildConfiguration);
void msgBuildAndRun(const std::string& inBuildConfiguration, const std::string& inName);
void msgBuild(const std::string& inBuildConfiguration, const std::string& inName);
void msgRebuild(const std::string& inBuildConfiguration, const std::string& inName);
void msgRun(const std::string& inBuildConfiguration, const std::string& inName);
void msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName);
void msgProfile(const std::string& inBuildConfiguration, const std::string& inName);

// void msgRemoving(const std::string& inPath);
void msgCopying(const std::string& inFrom, const std::string& inTo);
}
}

#endif // CHALET_OUTPUT_HPP
