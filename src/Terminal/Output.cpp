/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Core/CommandLineInputs.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Terminal/Commands.hpp"
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
static bool s_showCommands = false;
static bool s_allowCommandsToShow = true;
#if defined(CHALET_WIN32)
static std::int64_t s_commandPromptVersion = -1;
#endif

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
static ColorTheme sTheme;

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool Output::ansiColorsSupportedInComSpec()
{
	if (s_commandPromptVersion == -1)
	{
		s_commandPromptVersion = Commands::getLastWriteTime(Environment::getAsString("COMSPEC"));
		if (s_commandPromptVersion > 0)
			s_commandPromptVersion *= 1000;
	}

	// Note: ANSI terminal colors were added somewhere between Windows 10 build 10240 and 10586
	//   So we approximate based on the date of the first release of Windows 10 (build 10240, July 29, 2015)
	return s_commandPromptVersion > 1438128000000;
}
#endif
/*****************************************************************************/
void Output::setTheme(const ColorTheme& inTheme)
{
	if (inTheme != sTheme)
	{
		sTheme = inTheme;
	}
}
const ColorTheme& Output::theme()
{
	return sTheme;
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
bool Output::cleanOutput()
{
	return !s_showCommands || !s_allowCommandsToShow;
}

bool Output::showCommands()
{
	return s_allowCommandsToShow && s_showCommands;
}

void Output::setShowCommands(const bool inValue)
{
	s_showCommands = inValue;
}

void Output::setShowCommandOverride(const bool inValue)
{
	s_allowCommandsToShow = inValue;
}

/*****************************************************************************/
bool Output::getUserInput(const std::string& inUserQuery, std::string& outResult, const Color inAnswerColor, std::string note, const std::function<bool(std::string&)>& onValidate)
{
	const auto color = Output::getAnsiStyle(sTheme.flair);
	const auto noteColor = Output::getAnsiStyle(sTheme.note);
	const auto answerColor = Output::getAnsiStyle(inAnswerColor, true);
	const auto reset = Output::getAnsiReset();
	const char symbol = '>';

	const auto lineUp = fmt::format("{}[F", getEscapeChar());
	const std::string blankLine(80, ' ');
	const std::string cleanLine = fmt::format("{}\n{}", blankLine, lineUp);

	std::string output = fmt::format("{cleanLine}{color}{symbol}  {reset}{inUserQuery} ({outResult}) {answerColor}",
		FMT_ARG(cleanLine),
		FMT_ARG(color),
		FMT_ARG(symbol),
		FMT_ARG(reset),
		FMT_ARG(inUserQuery),
		FMT_ARG(outResult),
		FMT_ARG(answerColor));

	std::cout << fmt::format("\n   {noteColor}{note}{reset}{lineUp}{output}",
		FMT_ARG(noteColor),
		FMT_ARG(note),
		FMT_ARG(reset),
		FMT_ARG(lineUp),
		FMT_ARG(output));

	bool result = false;
	std::string input;
	if (std::getline(std::cin, input))
	{
		if (!input.empty() && onValidate(input))
		{
			outResult = std::move(input);
			result = true;
		}
	}

	auto toOutput = fmt::format("{cleanLine}{lineUp}{output}{outResult}{reset}",
		FMT_ARG(cleanLine),
		FMT_ARG(lineUp),
		FMT_ARG(output),
		FMT_ARG(outResult),
		FMT_ARG(reset));

	// toOutput.append(80 - toOutput.size(), ' ');
	std::cout << toOutput << std::endl;

	return result;
}

/*****************************************************************************/
bool Output::getUserInputYesNo(const std::string& inUserQuery, const Color inAnswerColor, std::string inNote)
{
	std::string result{ "yes" };
	return !getUserInput(inUserQuery, result, inAnswerColor, std::move(inNote), [](std::string& input) {
		return String::equals({ "no", "n" }, String::toLowerCase(input));
	});
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inColor, const bool inBold)
{
	const auto esc = getEscapeChar();
	char style = inBold ? '1' : '0';
	const int color = static_cast<std::underlying_type_t<Color>>(inColor);

#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
	{
		if (ansiColorsSupportedInComSpec())
		{
			// Note: Use the bright colors since they aren't as harsh
			//   Command Prompt bolding is all or nothing
			style = '1';
			return fmt::format("{esc}[{style}m{esc}[{color}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(color));
		}
		else
		{
			return std::string();
		}
	}
	else
#endif
	{
		return fmt::format("{esc}[{style};{color}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(color));
	}
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold)
{
	const auto esc = getEscapeChar();
	char style = inBold ? '1' : '0';
	const int fgColor = static_cast<std::underlying_type_t<Color>>(inForegroundColor);
	const int bgColor = static_cast<std::underlying_type_t<Color>>(inBackgroundColor) + 10;
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
	{
		if (ansiColorsSupportedInComSpec())
		{
			// Note: Use the bright colors since they aren't as harsh
			//   Command Prompt bolding is all or nothing
			style = '1';
			return fmt::format("{esc}[{style}m{esc}[{fgColor}m{esc}[{bgColor}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
		}
		else
		{
			return std::string();
		}
	}
	else
#endif
	{
		return fmt::format("{esc}[{style};{fgColor};{bgColor}m", FMT_ARG(esc), FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
	}
}

/*****************************************************************************/
std::string Output::getAnsiStyleUnescaped(const Color inColor, const bool inBold)
{
	char style = inBold ? '1' : '0';
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
	{
		if (ansiColorsSupportedInComSpec())
			style = '1';
	}
#endif
	const int color = static_cast<std::underlying_type_t<Color>>(inColor);

	return fmt::format("{style};{color}", FMT_ARG(style), FMT_ARG(color));
}

/*****************************************************************************/
std::string Output::getAnsiStyleUnescaped(const Color inForegroundColor, const Color inBackgroundColor, const bool inBold)
{
	char style = inBold ? '1' : '0';
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
	{
		if (ansiColorsSupportedInComSpec())
			style = '1';
	}
#endif
	const int fgColor = static_cast<std::underlying_type_t<Color>>(inForegroundColor);
	const int bgColor = static_cast<std::underlying_type_t<Color>>(inBackgroundColor) + 10;

	return fmt::format("{style};{fgColor};{bgColor}", FMT_ARG(style), FMT_ARG(fgColor), FMT_ARG(bgColor));
}

/*****************************************************************************/
std::string Output::getAnsiReset()
{
#if defined(CHALET_WIN32)
	if (Environment::isCommandPromptOrPowerShell() || Environment::isVisualStudioCommandPrompt())
	{
		if (!ansiColorsSupportedInComSpec())
		{
			return std::string();
		}
	}
#endif

	const auto esc = getEscapeChar();
	return fmt::format("{esc}[0m", FMT_ARG(esc));
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
void Output::lineBreakStderr()
{
	if (!s_quietNonBuild)
	{
		std::cerr << getAnsiReset() << std::endl;
	}
}

/*****************************************************************************/
void Output::previousLine()
{
	if (!s_quietNonBuild)
	{
		std::cout << fmt::format("{}[F", getEscapeChar()) << std::flush;
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
void Output::printCommand(const std::string& inText)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.build, false);
		const auto reset = getAnsiReset();
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printCommand(const StringList& inList)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.build, false);
		const auto reset = getAnsiReset();
		std::cout << color << String::join(inList) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printInfo(const std::string& inText)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.info, false);
		const auto reset = getAnsiReset();
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printFlair(const std::string& inText)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.flair, false);
		const auto reset = getAnsiReset();
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printSeparator(const char inChar)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.flair, false);
		const auto reset = getAnsiReset();
		std::cout << color << std::string(80, inChar) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty() && !String::equals("HEAD", inBranchOrTag))
		displayStyledSymbol(sTheme.alt, symbol, fmt::format("Fetching: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(sTheme.alt, symbol, fmt::format("Fetching: {}", path));
}

/*****************************************************************************/
void Output::msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty())
		displayStyledSymbol(sTheme.alt, symbol, fmt::format("Updating: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(sTheme.alt, symbol, fmt::format("Updating: {}", path));
}

/*****************************************************************************/
void Output::msgDisplayBlack(const std::string& inString)
{
	if (!s_quietNonBuild)
	{
		const auto color = getAnsiStyle(sTheme.flair, true);
		const auto reset = getAnsiReset();
		std::cout << color << fmt::format("   {}", inString) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::msgConfigureCompleted()
{
	auto symbol = Unicode::heavyCheckmark();
	displayStyledSymbol(sTheme.success, symbol, "Configured!");
}

/*****************************************************************************/
void Output::msgBuildSuccess()
{
	auto symbol = Unicode::heavyCheckmark();
	displayStyledSymbol(sTheme.success, symbol, "Succeeded!");
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName)
{
	if (!s_quietNonBuild)
	{
		std::string successText = "Target is up to date.";
		if (inMultiTarget)
			print(sTheme.build, fmt::format("   {}: {}", inProjectName, successText));
		else
			print(sTheme.build, fmt::format("   {}", successText));
	}
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	auto symbol = Unicode::heavyBallotX();
	displayStyledSymbol(sTheme.error, symbol, "Failed!");
	displayStyledSymbol(sTheme.error, " ", "Review the errors above.");
	// exit 1
}

/*****************************************************************************/
void Output::msgCleaning()
{
	print(sTheme.build, "   Removing build files & folders...");
}

/*****************************************************************************/
void Output::msgNothingToClean()
{
	print(sTheme.build, "   Nothing to clean...");
}

/*****************************************************************************/
void Output::msgCleaningRebuild()
{
	print(sTheme.build, "   Removing previous build files & folders...");
}

/*****************************************************************************/
void Output::msgBuildProdError(const std::string& inBuildConfiguration)
{
	auto symbol = Unicode::circledSaltire();
	displayStyledSymbol(sTheme.error, symbol, fmt::format("Error: 'bundle' must be run on '{}' build.", inBuildConfiguration));
	// exit 1
}

/*****************************************************************************/
void Output::msgProfilerStartedGprof(const std::string& inProfileAnalysis)
{
	Diagnostic::info("Writing profiling analysis to {}. This may take a while...", inProfileAnalysis);
}

/*****************************************************************************/
void Output::msgProfilerStartedSample(const std::string& inExecutable, const uint inDuration, const uint inSamplingInterval)
{
	Diagnostic::info("Sampling {} for {} seconds with {} millisecond of run time between samples", inExecutable, inDuration, inSamplingInterval);
}

/*****************************************************************************/
void Output::msgProfilerDone(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(sTheme.alt, symbol, fmt::format("Profiler Completed! View {} for details.", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerDoneInstruments(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(sTheme.alt, symbol, fmt::format("Profiler Completed! Launching {} in Instruments.", inProfileAnalysis));
}

// Leave the commands as separate functions in case symbols and things change

/*****************************************************************************/

void Output::msgClean(const std::string& inBuildConfiguration)
{
	auto symbol = Unicode::triangle();
	if (!inBuildConfiguration.empty())
		displayStyledSymbol(sTheme.header, symbol, "Clean: " + inBuildConfiguration);
	else
		displayStyledSymbol(sTheme.header, symbol, "Clean: All");
}

/*****************************************************************************/
void Output::msgBuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(sTheme.header, symbol, "Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgRebuild(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(sTheme.header, symbol, "Rebuild: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgScript(const std::string& inName, const Color inColor)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(inColor, symbol, fmt::format("Script: {}", inName));
}

/*****************************************************************************/
void Output::msgTargetDescription(const std::string& inDescription, const Color inColor)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(inColor, symbol, inDescription);
}

/*****************************************************************************/
void Output::msgRun(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(sTheme.success, symbol, "Run: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgBuildProd(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(sTheme.header, symbol, "Production Build: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgProfile(const std::string& inBuildConfiguration, const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(sTheme.header, symbol, "Profile: " + getFormattedBuildTarget(inBuildConfiguration, inName));
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	auto symbol = Unicode::heavyCurvedUpRightArrow();
	std::string message = fmt::format("Copying: '{}' to '{}'", inFrom, inTo);

	displayStyledSymbol(sTheme.build, symbol, message, false);
}
}