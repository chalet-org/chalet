/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
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

#if !defined(CHALET_WIN32)
/*****************************************************************************/
std::string getAnsiStyle(const Color inColor, const bool inBold = false)
{
	const auto style = inBold ? "1" : "0";
	const int color = static_cast<std::underlying_type_t<Color>>(inColor);
	return fmt::format("\033[{style};{color}m", FMT_ARG(style), FMT_ARG(color));
}

/*****************************************************************************/
std::string_view getAnsiReset()
{
	return "\033[0m";
}
#endif

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
	std::string searchChar = "/";
	if (String::contains(":", ret))
		searchChar = ":";

	std::size_t beg = ret.find_first_of(searchChar);
	if (beg != std::string::npos)
	{
		ret = ret.substr(beg + 1);
	}

	// strip .git
	std::size_t end = ret.find_last_of(".");
	if (end != std::string::npos)
	{
		ret = ret.substr(0, end);
	}

	return ret;
}
}

/*****************************************************************************/
void Output::displayStyledSymbol(const Color inColor, const std::string& inSymbol, const std::string& inMessage, const bool inBold)
{
#if defined(CHALET_WIN32)
	if (inBold)
		std::cout << rang::style::bold << inColor << fmt::format("{}  {}", inSymbol, inMessage) << rang::style::reset << std::endl;
	else
		std::cout << inColor << fmt::format("{}  {}", inSymbol, inMessage) << rang::style::reset << std::endl;
#else
	const auto color = getAnsiStyle(inColor, inBold);
	const auto reset = getAnsiReset();
	std::cout << fmt::format("{color}{inSymbol}  {inMessage}", FMT_ARG(color), FMT_ARG(inSymbol), FMT_ARG(inMessage)) << reset << std::endl;
#endif
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
void Output::reset()
{
#if defined(CHALET_WIN32)
	std::cout << rang::style::reset << std::endl;
#else
	std::cout << getAnsiReset() << std::endl;
#endif
}

/*****************************************************************************/
void Output::lineBreak()
{
#if defined(CHALET_WIN32)
	std::cout << rang::style::reset << std::endl;
#else
	std::cout << getAnsiReset() << std::endl;
#endif
}

/*****************************************************************************/
void Output::print(const Color inColor, const std::string& inText)
{
#if defined(CHALET_WIN32)
	std::cout << inColor << inText << rang::style::reset << std::endl;
#else
	const auto color = getAnsiStyle(inColor);
	const auto reset = getAnsiReset();
	std::cout << color << inText << reset << std::endl;
#endif
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	constexpr auto symbol = Unicode::heavyCurvedDownRightArrow();

	displayStyledSymbol(Color::magenta, symbol, fmt::format("Fetching: {} ({})", path, inBranchOrTag));
}

void Output::msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	constexpr auto symbol = Unicode::heavyCurvedDownRightArrow();

	displayStyledSymbol(Color::magenta, symbol, fmt::format("Updating: {} ({})", path, inBranchOrTag));
}

/*****************************************************************************/
void Output::msgDisplayBlack(const std::string& inString)
{
#if defined(CHALET_WIN32)
	std::cout << rang::fg::black << rang::style::bold << fmt::format("   {}", inString) << rang::style::reset << std::endl;
#else
	const auto color = getAnsiStyle(Color::black, true);
	const auto reset = getAnsiReset();
	std::cout << color << inString << reset << std::endl;
#endif
}

/*****************************************************************************/
void Output::msgBuildSuccess()
{
	constexpr auto symbol = Unicode::heavyCheckmark();
	displayStyledSymbol(Color::green, symbol, "Succeeded!");
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName)
{
	std::string successText = "Target is up to date.";
	if (inMultiTarget)
		print(Color::blue, fmt::format("   {}: {}", inProjectName, successText));
	else
		print(Color::blue, fmt::format("   {}", successText));
}

/*****************************************************************************/
void Output::msgLaunch(const std::string& inBuildDir, const std::string& inName)
{
	displayStyledSymbol(Color::green, " ", fmt::format("Launching {}/{}", inBuildDir, inName));
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	constexpr auto symbol = Unicode::heavyBallotX();
	displayStyledSymbol(Color::red, symbol, "Failed!");
	displayStyledSymbol(Color::red, " ", "Review the compile errors above.");
	// exit 1
}

/*****************************************************************************/
void Output::msgCleaning()
{
	print(Color::blue, "   Removing build files & folders...");
}

/*****************************************************************************/
void Output::msgNothingToClean()
{
	print(Color::blue, "   Nothing to clean...");
}

/*****************************************************************************/
void Output::msgCleaningRebuild()
{
	print(Color::blue, "   Removing previous build files & folders...");
}

/*****************************************************************************/
void Output::msgBuildProdError(const std::string& inBuildConfiguration)
{
	constexpr auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(Color::red, symbol, fmt::format("Error: 'bundle' must be run on '{}' build.", inBuildConfiguration));
	// exit 1
}

/*****************************************************************************/
void Output::msgProfilerDone(const std::string& inProfileAnalysis)
{
	constexpr auto symbol = Unicode::diamond();
	displayStyledSymbol(Color::magenta, symbol, fmt::format("Profiler Completed: View {} for details.", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerError()
{
	constexpr auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(Color::red, symbol, "Error: Profiler must be run on 'Debug' build.");
	// exit 1
}

/*****************************************************************************/
void Output::msgProfilerErrorMacOS()
{
	constexpr auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(Color::red, symbol, "Error: Profiling (with gprof) is not supported on MacOS.");
	// exit 1
}

// Leave the commands as separate functions in case symbols and things change

/*****************************************************************************/

void Output::msgClean(const std::string& inBuildConfiguration)
{
	constexpr auto symbol = Unicode::dot();
	if (!inBuildConfiguration.empty())
		displayStyledSymbol(Color::yellow, symbol, "Clean: " + inBuildConfiguration);
	else
		displayStyledSymbol(Color::yellow, symbol, "Clean: All");
}

/*****************************************************************************/
void Output::msgBuildAndRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Build & Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRebuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Rebuild: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Production Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgProfile(const std::string& inBuildConfiguration, const std::string& inName)
{
	constexpr auto symbol = Unicode::dot();
	displayStyledSymbol(Color::yellow, symbol, "Profile: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	constexpr auto symbol = Unicode::heavyCurvedUpRightArrow();
	std::string message = fmt::format("Copying: '{}' to '{}'", inFrom, inTo);

	displayStyledSymbol(Color::blue, symbol, message, false);
}

}