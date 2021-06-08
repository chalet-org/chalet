/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Libraries/WindowsApi.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
static bool s_quietNonBuild = false;

/*****************************************************************************/
std::string getFormattedBuildTarget(const std::string& inBuildConfiguration, const std::string& inName)
{
	return fmt::format("{} (target: {})", inBuildConfiguration, inName);
}

/*****************************************************************************/
// TODO: move this
std::string getCleanGitPath(const std::string inPath)
{
	std::string ret = inPath;

	// Common git patterns
	String::replaceAll(ret, "https://", "");
	String::replaceAll(ret, "git@", "");
	String::replaceAll(ret, "git+ssh://", "");
	String::replaceAll(ret, "ssh://", "");
	String::replaceAll(ret, "git://", "");
	// String::replaceAll(ret, "rsync://", "");
	// String::replaceAll(ret, "file://", "");

	// strip the domain
	char searchChar = '/';
	if (String::contains(':', ret))
		searchChar = ':';

	std::size_t beg = ret.find_first_of(searchChar);
	if (beg != std::string::npos)
	{
		ret = ret.substr(beg + 1);
	}

	// strip .git
	std::size_t end = ret.find_last_of('.');
	if (end != std::string::npos)
	{
		ret = ret.substr(0, end);
	}

	return ret;
}

/*****************************************************************************/
char getEscapeChar()
{
#if defined(CHALET_WIN32)
	return '\x1b';
#else
	return '\033';
#endif
	// if (Environment::isBash())
	// 	return '\033';
	// else
	// 	return '\x1b';
}
}

/*****************************************************************************/
bool Output::quietNonBuild()
{
	return s_quietNonBuild;
}

void Output::setQuietNonBuild(const bool inValue)
{
	s_quietNonBuild = inValue;
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inColor, const bool inBold)
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
	{
		return std::string();
	}
	else
	{
#endif
		const auto esc = getEscapeChar();
		const char style = inBold ? '1' : '0';
		const int color = static_cast<std::underlying_type_t<Color>>(inColor);

		return fmt::format("{esc}[{style};{color}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(color));
#if defined(CHALET_WIN32)
	}
#endif
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold)
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
	{
		return std::string();
	}
	else
	{
#endif
		const auto esc = getEscapeChar();
		const char style = inBold ? '1' : '0';
		const int fgColor = static_cast<std::underlying_type_t<Color>>(inForegroundColor);
		const int bgColor = static_cast<std::underlying_type_t<Color>>(inBackgroundColor) + 10;

		return fmt::format("{esc}[{style};{fgColor};{bgColor}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
#if defined(CHALET_WIN32)
	}
#endif
}

/*****************************************************************************/
std::string Output::getAnsiReset()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell())
	{
		return std::string();
	}
	else
	{
#endif
		const auto esc = getEscapeChar();
		return fmt::format("{esc}[0m", FMT_ARG(esc));
#if defined(CHALET_WIN32)
	}
#endif
}

/*****************************************************************************/
void Output::displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage, const bool inBold)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(inColor, inBold);
		const auto reset = getAnsiReset();
		std::cout << fmt::format("{color}{inSymbol}  {inMessage}", FMT_ARG(color), FMT_ARG(inSymbol), FMT_ARG(inMessage)) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::resetStdout()
{
	std::cout << getAnsiReset() << std::flush;
}

/*****************************************************************************/
void Output::resetStderr()
{
	std::cerr << getAnsiReset() << std::flush;
}

/*****************************************************************************/
void Output::lineBreak()
{
	if (!s_quietNonBuild)
	{
		std::cout << getAnsiReset() << std::endl;
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const std::string& inText, const bool inBold)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(inColor, inBold);
		const auto reset = getAnsiReset();
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const StringList& inList, const bool inBold)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(inColor, inBold);
		const auto reset = getAnsiReset();
		std::cout << color << String::join(inList) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty())
		displayStyledSymbol(Color::Magenta, symbol, fmt::format("Fetching: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(Color::Magenta, symbol, fmt::format("Fetching: {}", path));
}

void Output::msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty())
		displayStyledSymbol(Color::Magenta, symbol, fmt::format("Updating: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(Color::Magenta, symbol, fmt::format("Updating: {}", path));
}

/*****************************************************************************/
void Output::msgDisplayBlack(const std::string& inString)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(Color::Black, true);
		const auto reset = getAnsiReset();
		std::cout << color << fmt::format("   {}", inString) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::msgBuildSuccess()
{
	auto symbol = Unicode::heavyCheckmark();
	displayStyledSymbol(Color::Green, symbol, "Succeeded!");
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName)
{
	if (!s_quietNonBuild)
	{
		std::string successText = "Target is up to date.";
		if (inMultiTarget)
			print(Color::Blue, fmt::format("   {}: {}", inProjectName, successText));
		else
			print(Color::Blue, fmt::format("   {}", successText));
	}
}

/*****************************************************************************/
void Output::msgLaunch(const std::string& inBuildDir, const std::string& inName)
{
	displayStyledSymbol(Color::Green, " ", fmt::format("Launching {}/{}", inBuildDir, inName));
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	auto symbol = Unicode::heavyBallotX();
	displayStyledSymbol(Color::Red, symbol, "Failed!");
	displayStyledSymbol(Color::Red, " ", "Review the errors above.");
	// exit 1
}

/*****************************************************************************/
void Output::msgCleaning()
{
	print(Color::Blue, "   Removing build files & folders...");
}

/*****************************************************************************/
void Output::msgNothingToClean()
{
	print(Color::Blue, "   Nothing to clean...");
}

/*****************************************************************************/
void Output::msgCleaningRebuild()
{
	print(Color::Blue, "   Removing previous build files & folders...");
}

/*****************************************************************************/
void Output::msgBuildProdError(const std::string& inBuildConfiguration)
{
	auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(Color::Red, symbol, fmt::format("Error: 'bundle' must be run on '{}' build.", inBuildConfiguration));
	// exit 1
}

/*****************************************************************************/
void Output::msgProfilerStartedGprof(const std::string& inProfileAnalysis)
{
	print(Color::Gray, fmt::format("   Writing profiling analysis to {}. This may take a while...\n", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerStartedSample(const std::string& inExecutable, const uint inDuration, const uint inSamplingInterval)
{
	print(Color::Gray, fmt::format("   Sampling {} for {} seconds with {} millisecond of run time between samples", inExecutable, inDuration, inSamplingInterval));
}

/*****************************************************************************/
void Output::msgProfilerDone(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(Color::Magenta, symbol, fmt::format("Profiler Completed! View {} for details.", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerDoneInstruments(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(Color::Magenta, symbol, fmt::format("Profiler Completed! Launching {} in Instruments.", inProfileAnalysis));
}

// Leave the commands as separate functions in case symbols and things change

/*****************************************************************************/

void Output::msgClean(const std::string& inBuildConfiguration)
{
	auto symbol = Unicode::triangle();
	if (!inBuildConfiguration.empty())
		displayStyledSymbol(Color::Yellow, symbol, "Clean: " + inBuildConfiguration);
	else
		displayStyledSymbol(Color::Yellow, symbol, "Clean: All");
}

/*****************************************************************************/
void Output::msgBuildAndRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Build & Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRebuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Rebuild: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgScript(const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, fmt::format("Script: {}", inName));
}

/*****************************************************************************/
void Output::msgScriptDescription(const std::string& inDescription)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, inDescription);
}

/*****************************************************************************/
void Output::msgRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Production Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgProfile(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(Color::Yellow, symbol, "Profile: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	auto symbol = Unicode::heavyCurvedUpRightArrow();
	std::string message = fmt::format("Copying: '{}' to '{}'", inFrom, inTo);

	displayStyledSymbol(Color::Blue, symbol, message, false);
}

}