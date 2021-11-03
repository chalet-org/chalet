/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

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
static struct
{
	ColorTheme theme;

	bool quietNonBuild = false;
	bool showCommands = false;
	bool allowCommandsToShow = true;
	bool showBenchamrks = true;
#if defined(CHALET_WIN32)
	std::int64_t commandPromptVersion = -1;
#endif
} state;

/*****************************************************************************/
std::string getFormattedBuildTarget(const std::string& inName)
{
	return fmt::format(": {}", inName);
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
constexpr char getEscapeChar()
{
#if defined(CHALET_WIN32)
	return '\x1b';
#else
	return '\033';
#endif
}
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool Output::ansiColorsSupportedInComSpec()
{
	if (state.commandPromptVersion == -1)
	{
		state.commandPromptVersion = Commands::getLastWriteTime(Environment::getAsString("COMSPEC"));
		if (state.commandPromptVersion > 0)
			state.commandPromptVersion *= 1000;
	}

	// Note: ANSI terminal colors were added somewhere between Windows 10 build 10240 and 10586
	//   So we approximate based on the date of the first release of Windows 10 (build 10240, July 29, 2015)
	return state.commandPromptVersion > 1438128000000;
}
#endif
/*****************************************************************************/
void Output::setTheme(const ColorTheme& inTheme)
{
	if (inTheme != state.theme)
	{
		state.theme = inTheme;
	}
}
const ColorTheme& Output::theme()
{
	return state.theme;
}

/*****************************************************************************/
bool Output::quietNonBuild()
{
	return state.quietNonBuild;
}

void Output::setQuietNonBuild(const bool inValue)
{
	state.quietNonBuild = inValue;
}

/*****************************************************************************/
bool Output::cleanOutput()
{
	return !state.showCommands || !state.allowCommandsToShow;
}

bool Output::showCommands()
{
	return state.allowCommandsToShow && state.showCommands;
}

void Output::setShowCommands(const bool inValue)
{
	state.showCommands = inValue;
}

void Output::setShowCommandOverride(const bool inValue)
{
	state.allowCommandsToShow = inValue;
}

/*****************************************************************************/
bool Output::showBenchmarks()
{
	return state.showBenchamrks;
}

void Output::setShowBenchmarks(const bool inValue)
{
	state.showBenchamrks = inValue;
}

/*****************************************************************************/
bool Output::getUserInput(const std::string& inUserQuery, std::string& outResult, std::string note, const std::function<bool(std::string&)>& onValidate)
{
	const auto color = Output::getAnsiStyle(state.theme.flair);
	const auto noteColor = Output::getAnsiStyle(state.theme.build);
	const auto answerColor = Output::getAnsiStyle(state.theme.note);
	const auto reset = Output::getAnsiStyle(Color::Reset);
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

	std::string input;
	std::getline(std::cin, input); // get up to first line break (if applicable)

	if (input.empty())
		input = outResult;
	else
		outResult = input;

	bool result = onValidate(input);

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
bool Output::getUserInputYesNo(const std::string& inUserQuery, const bool inDefaultYes, std::string inNote)
{
	std::string result{ inDefaultYes ? "yes" : "no" };
	return getUserInput(inUserQuery, result, std::move(inNote), [](std::string& input) {
		return !String::equals({ "no", "n" }, String::toLowerCase(input));
	});
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inColor)
{
	if (inColor == Color::None)
		return std::string();

#if defined(CHALET_WIN32)
	bool isCmdPromptLike = Environment::isCommandPromptOrPowerShell();
	if (isCmdPromptLike && !ansiColorsSupportedInComSpec())
		return std::string();
#endif

	constexpr char esc = getEscapeChar();
	if (inColor == Color::Reset)
		return fmt::format("{}[0m", esc);

	using ColorType = std::underlying_type_t<Color>;
	ColorType color = static_cast<ColorType>(inColor);
	ColorType style = color / static_cast<ColorType>(100);
	if (color > 100)
		color -= (style * static_cast<ColorType>(100));

#if defined(CHALET_WIN32)
	if (isCmdPromptLike)
		return fmt::format("{}[{}m{}[{}m", esc, style, esc, color);
#endif

	return fmt::format("{}[{};{}m", esc, style, color);
}

/*****************************************************************************/
std::string Output::getAnsiStyleRaw(const Color inColor)
{
	if (inColor == Color::None)
		return std::string();

#if defined(CHALET_WIN32)
	bool isCmdPromptLike = Environment::isCommandPromptOrPowerShell();
	if (isCmdPromptLike && !ansiColorsSupportedInComSpec())
		return std::string();
#endif

	if (inColor == Color::Reset)
		return "0";

	using ColorType = std::underlying_type_t<Color>;
	ColorType color = static_cast<ColorType>(inColor);
	ColorType style = color / static_cast<ColorType>(100);
	if (color > 100)
		color -= (style * static_cast<ColorType>(100));

	return fmt::format("{};{}", style, color);
}

/*****************************************************************************/
std::string Output::getAnsiStyleForMakefile(const Color inColor)
{
#if defined(CHALET_WIN32)
	return getAnsiStyle(inColor);
#else
	auto ret = getAnsiStyle(inColor);
	String::replaceAll(ret, getEscapeChar(), "\\033");
	return ret;
#endif
}

/*****************************************************************************/
std::string Output::getAnsiStyleForceFormatting(const Color inColor, const Formatting inFormatting)
{
	using ColorType = std::underlying_type_t<Color>;
	ColorType color = static_cast<ColorType>(inColor);
	ColorType baseColor = color % static_cast<ColorType>(100);
	ColorType style = static_cast<ColorType>(inFormatting);
	color = style + baseColor;

	return getAnsiStyle(static_cast<Color>(color));
}

/*****************************************************************************/
void Output::displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(inColor);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << fmt::format("{}{}  {}", color, inSymbol, inMessage) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::resetStdout()
{
	auto reset = getAnsiStyle(Color::Reset);
	if (!reset.empty())
		std::cout << reset << std::flush;
}

/*****************************************************************************/
void Output::resetStderr()
{
	auto reset = getAnsiStyle(Color::Reset);
	if (!reset.empty())
		std::cerr << reset << std::flush;
}

/*****************************************************************************/
void Output::lineBreak()
{
	if (!state.quietNonBuild)
	{
		std::cout << getAnsiStyle(Color::Reset) << std::endl;
	}
}

/*****************************************************************************/
void Output::lineBreakStderr()
{
	if (!state.quietNonBuild)
	{
		std::cerr << getAnsiStyle(Color::Reset) << std::endl;
	}
}

/*****************************************************************************/
void Output::previousLine()
{
	if (!state.quietNonBuild)
	{
		std::cout << fmt::format("{}[F", getEscapeChar()) << std::flush;
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto reset = getAnsiStyle(Color::Reset);
		if (inColor == Color::Reset)
		{
			std::cout << reset << inText << std::endl;
		}
		else
		{
			const auto color = getAnsiStyle(inColor);
			std::cout << color << inText << reset << std::endl;
		}
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto reset = getAnsiStyle(Color::Reset);
		if (inColor == Color::Reset)
		{
			std::cout << reset << String::join(inList) << std::endl;
		}
		else
		{
			const auto color = getAnsiStyle(inColor);
			std::cout << color << String::join(inList) << reset << std::endl;
		}
	}
}

/*****************************************************************************/
void Output::printCommand(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.build);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printCommand(const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.build);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << color << String::join(inList) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printInfo(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.info);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printFlair(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.flair);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << color << inText << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::printSeparator(const char inChar)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.flair);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << color << std::string(80, inChar) << reset << std::endl;
	}
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty() && !String::equals("HEAD", inBranchOrTag))
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Fetching: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Fetching: {}", path));
}

/*****************************************************************************/
void Output::msgUpdatingDependency(const std::string& inGitUrl, const std::string& inBranchOrTag)
{
	std::string path = getCleanGitPath(inGitUrl);
	auto symbol = Unicode::heavyCurvedDownRightArrow();

	if (!inBranchOrTag.empty())
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Updating: {} ({})", path, inBranchOrTag));
	else
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Updating: {}", path));
}

/*****************************************************************************/
void Output::msgRemovedUnusedDependency(const std::string& inDependencyName)
{
	print(state.theme.flair, fmt::format("   Removed unused dependency: '{}'", inDependencyName));
}

/*****************************************************************************/
void Output::msgConfigureCompleted(const std::string& inWorkspaceName)
{
	auto symbol = Unicode::checkmark();
	displayStyledSymbol(state.theme.success, symbol, fmt::format("The '{}' workspace has been configured!", inWorkspaceName));
}

/*****************************************************************************/
void Output::msgBuildSuccess()
{
	auto symbol = Unicode::checkmark();
	displayStyledSymbol(state.theme.success, symbol, "Succeeded!");
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const bool inMultiTarget, const std::string& inProjectName)
{
	if (!state.quietNonBuild)
	{
		std::string successText = "Target is up to date.";
		if (inMultiTarget)
			print(state.theme.build, fmt::format("   {}: {}", inProjectName, successText));
		else
			print(state.theme.build, fmt::format("   {}", successText));
	}
}

/*****************************************************************************/
void Output::msgCommandPoolError(const std::string& inMessage)
{
	if (!state.quietNonBuild)
	{
		const auto colorError = getAnsiStyle(state.theme.error);
		const auto reset = getAnsiStyle(Color::Reset);
		std::cout << fmt::format("{}FAILED: {}{}", colorError, reset, inMessage) << std::endl;
	}
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	// Always display this (don't handle state.quietNonBuild)
	//
	auto symbol = Unicode::heavyBallotX();

	const auto color = getAnsiStyle(state.theme.error);
	const auto reset = getAnsiStyle(Color::Reset);

	std::cout << fmt::format("{}{}  Failed!\n   Review the errors above.{}", color, symbol, reset) << std::endl;
}

/*****************************************************************************/
void Output::msgCleaning()
{
	print(state.theme.build, "   Removing build files & folders...");
}

/*****************************************************************************/
void Output::msgNothingToClean()
{
	print(state.theme.build, "   Nothing to clean...");
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
	displayStyledSymbol(state.theme.note, symbol, fmt::format("Profiler Completed! View {} for details.", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerDoneInstruments(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(state.theme.note, symbol, fmt::format("Profiler Completed! Launching {} in Instruments.", inProfileAnalysis));
}

// Leave the commands as separate functions in case symbols and things change

/*****************************************************************************/

void Output::msgClean(const std::string& inBuildConfiguration)
{
	auto symbol = Unicode::triangle();
	if (!inBuildConfiguration.empty())
		displayStyledSymbol(state.theme.header, symbol, "Clean: " + inBuildConfiguration);
	else
		displayStyledSymbol(state.theme.header, symbol, "Clean: All");
}

/*****************************************************************************/
void Output::msgBuild(const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(state.theme.header, symbol, "Build" + getFormattedBuildTarget(inName));
}

/*****************************************************************************/
void Output::msgRebuild(const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(state.theme.header, symbol, "Rebuild" + getFormattedBuildTarget(inName));
}

/*****************************************************************************/
void Output::msgDistribution(const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(state.theme.header, symbol, fmt::format("Distribution: {}", inName));
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
void Output::msgRun(const std::string& inName)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(state.theme.success, symbol, "Run" + getFormattedBuildTarget(inName));
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	auto symbol = Unicode::heavyCurvedUpRightArrow();
	std::string message = fmt::format("Copying: '{}' to '{}'", inFrom, inTo);

	displayStyledSymbol(state.theme.build, symbol, message);
}
}