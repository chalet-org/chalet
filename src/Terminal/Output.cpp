/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Libraries/WindowsApi.hpp"
#include "State/CommandLineInputs.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
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
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inColor, const bool inBold)
{
	const char style = inBold ? '1' : '0';
	const int color = static_cast<std::underlying_type_t<Color>>(inColor);

	if (Environment::isBash())
	{
		return fmt::format("\033[{style};{color}m", FMT_ARG(style), FMT_ARG(color));
	}
	else
	{
		return fmt::format("\x1b[{style};{color}m", FMT_ARG(style), FMT_ARG(color));
	}
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold)
{
	const char style = inBold ? '1' : '0';
	const int fgColor = static_cast<std::underlying_type_t<Color>>(inForegroundColor);
	const int bgColor = static_cast<std::underlying_type_t<Color>>(inBackgroundColor) + 10;

	if (Environment::isBash())
	{
		return fmt::format("\033[{style};{fgColor};{bgColor}m", FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
	}
	else
	{
		return fmt::format("\x1b[{style};{fgColor};{bgColor}m", FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
	}
}

/*****************************************************************************/
std::string_view Output::getAnsiReset()
{
	if (Environment::isBash())
	{
		return "\033[0m";
	}
	else
	{
		return "\x1b[0m";
	}
}

/*****************************************************************************/
void Output::displayStyledSymbol(const Color inColor, const std::string& inSymbol, const std::string& inMessage, const bool inBold)
{
	const auto color = getAnsiStyle(inColor, inBold);
	const auto reset = getAnsiReset();
	std::cout << fmt::format("{color}{inSymbol}  {inMessage}", FMT_ARG(color), FMT_ARG(inSymbol), FMT_ARG(inMessage)) << reset << std::endl;
}

/*****************************************************************************/
void Output::warnBlankKey(const std::string& inKey, const std::string& inDefault)
{
	if (!inDefault.empty())
		Diagnostic::warn(fmt::format("{}: '{}' was defined, but blank. Using the built-in default ({})", CommandLineInputs::file(), inKey, inDefault));
	else
		Diagnostic::warn(fmt::format("{}: '{}' was defined, but blank.", CommandLineInputs::file(), inKey));
}

/*****************************************************************************/
void Output::warnBlankKeyInList(const std::string& inKey)
{
	Diagnostic::warn(fmt::format("{}: A blank value was found in '{}'.", CommandLineInputs::file(), inKey));
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
	std::cout << getAnsiReset() << std::endl;
}

/*****************************************************************************/
void Output::print(const Color inColor, const std::string& inText)
{
	const auto color = getAnsiStyle(inColor);
	const auto reset = getAnsiReset();
	std::cout << color << inText << reset << std::endl;
}

/*****************************************************************************/
void Output::print(const Color inColor, const StringList& inList)
{
	const auto color = getAnsiStyle(inColor);
	const auto reset = getAnsiReset();
	std::cout << color;
	for (auto& item : inList)
	{
		std::ptrdiff_t i = &item - &inList.front();
		std::cout << item;
		if (static_cast<std::size_t>(i) < inList.size() - 1)
			std::cout << ' ';
	}
	std::cout << reset << std::endl;
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	constexpr auto symbol = Unicode::heavyCurvedDownRightArrow();

	displayStyledSymbol(Color::Magenta, symbol, fmt::format("Fetching: {} ({})", path, inBranchOrTag));
}

void Output::msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	constexpr auto symbol = Unicode::heavyCurvedDownRightArrow();

	displayStyledSymbol(Color::Magenta, symbol, fmt::format("Updating: {} ({})", path, inBranchOrTag));
}

/*****************************************************************************/
void Output::msgDisplayBlack(const std::string& inString)
{
	const auto color = getAnsiStyle(Color::Black, true);
	const auto reset = getAnsiReset();
	std::cout << color << fmt::format("   {}", inString) << reset << std::endl;
}

/*****************************************************************************/
void Output::msgBuildSuccess()
{
	constexpr auto symbol = Unicode::heavyCheckmark();
	displayStyledSymbol(Color::Green, symbol, "Succeeded!");
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName)
{
	std::string successText = "Target is up to date.";
	if (inMultiTarget)
		print(Color::Blue, fmt::format("   {}: {}", inProjectName, successText));
	else
		print(Color::Blue, fmt::format("   {}", successText));
}

/*****************************************************************************/
void Output::msgLaunch(const std::string& inBuildDir, const std::string& inName)
{
	displayStyledSymbol(Color::Green, " ", fmt::format("Launching {}/{}", inBuildDir, inName));
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	constexpr auto symbol = Unicode::heavyBallotX();
	displayStyledSymbol(Color::Red, symbol, "Failed!");
	displayStyledSymbol(Color::Red, " ", "Review the compile errors above.");
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
	constexpr auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(Color::Red, symbol, fmt::format("Error: 'bundle' must be run on '{}' build.", inBuildConfiguration));
	// exit 1
}

/*****************************************************************************/
void Output::msgProfilerStarted(const std::string& inProfileAnalysis)
{
	print(Color::Gray, fmt::format("   Writing profiling analysis to {}. This may take a while...\n", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerDone(const std::string& inProfileAnalysis)
{
	constexpr auto symbol = Unicode::diamond();
	displayStyledSymbol(Color::Magenta, symbol, fmt::format("Profiler Completed! View {} for details.", inProfileAnalysis));
}

// Leave the commands as separate functions in case symbols and things change

/*****************************************************************************/

void Output::msgClean(const std::string& inBuildConfiguration)
{
	constexpr auto symbol = Unicode::dot();
	if (!inBuildConfiguration.empty())
		displayStyledSymbol(Color::Yellow, symbol, "Clean: " + inBuildConfiguration);
	else
		displayStyledSymbol(Color::Yellow, symbol, "Clean: All");
}

/*****************************************************************************/
void Output::msgBuildAndRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Build & Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRebuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Rebuild: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Production Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgProfile(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::Yellow, symbol, "Profile: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	constexpr auto symbol = Unicode::heavyCurvedUpRightArrow();
	std::string message = fmt::format("Copying: '{}' to '{}'", inFrom, inTo);

	displayStyledSymbol(Color::Blue, symbol, message, false);
}

}