/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Process/Environment.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
#include "Terminal/Shell.hpp"
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
	i64 commandPromptVersion = -1;
#endif
} state;

/*****************************************************************************/
std::string getFormattedBuildTarget(const std::string& inName)
{
	return fmt::format(": {}", inName);
}

/*****************************************************************************/
constexpr char getEscapeChar()
{
	return '\x1b';
}
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool Output::ansiColorsSupportedInComSpec()
{
	if (state.commandPromptVersion == -1)
	{
		state.commandPromptVersion = Files::getLastWriteTime(Environment::getString("COMSPEC"));
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

std::ostream& Output::getErrStream()
{
#if defined(CHALET_WIN32)
	if (Shell::isVisualStudioOutput())
		return std::cout;
#endif

	return std::cerr;
}

/*****************************************************************************/
bool Output::getUserInput(const std::string& inUserQuery, std::string& outResult, std::string note, const std::function<bool(std::string&)>& onValidate, const bool inFailOnFalse)
{
	const auto color = Output::getAnsiStyle(state.theme.flair);
	const auto noteColor = Output::getAnsiStyle(state.theme.build);
	const auto answerColor = Output::getAnsiStyle(state.theme.note);
	const auto reset = Output::getAnsiStyle(state.theme.reset);
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

	auto withNote = fmt::format("\n   {noteColor}{note}{reset}{lineUp}{output}",
		FMT_ARG(noteColor),
		FMT_ARG(note),
		FMT_ARG(reset),
		FMT_ARG(lineUp),
		FMT_ARG(output));

	std::cout.write(withNote.data(), withNote.size());

	std::string input;
	std::getline(std::cin, input); // get up to first line break (if applicable)

	if (input.empty())
		input = outResult;

	bool result = onValidate(input);
	if (!result && inFailOnFalse)
	{
		const auto error = Output::getAnsiStyle(state.theme.error);
		auto invalid = fmt::format("{cleanLine}{lineUp}{output}{input}{color} -- {error}invalid entry{reset}",
			FMT_ARG(cleanLine),
			FMT_ARG(lineUp),
			FMT_ARG(output),
			FMT_ARG(input),
			FMT_ARG(color),
			FMT_ARG(error),
			FMT_ARG(reset));

		std::cout.write(invalid.data(), invalid.size());
		std::cout.put('\n');
		std::cout.flush();

		return getUserInput(inUserQuery, outResult, std::move(note), onValidate, inFailOnFalse);
	}
	else
	{
		outResult = input;

		auto toOutput = fmt::format("{cleanLine}{lineUp}{output}{outResult}{reset}",
			FMT_ARG(cleanLine),
			FMT_ARG(lineUp),
			FMT_ARG(output),
			FMT_ARG(outResult),
			FMT_ARG(reset));

		// toOutput.append(80 - toOutput.size(), ' ');
		std::cout.write(toOutput.data(), toOutput.size());
		std::cout.put('\n');
		std::cout.flush();

		return result;
	}
}

/*****************************************************************************/
bool Output::getUserInputYesNo(const std::string& inUserQuery, const bool inDefaultYes, std::string inNote)
{
	std::string result{ inDefaultYes ? "yes" : "no" };
	return getUserInput(
		inUserQuery, result, std::move(inNote), [](std::string& input) {
			bool isYes = !String::equals(StringList{ "no", "n" }, String::toLowerCase(input));

			if (isYes)
				input = "yes";
			else
				input = "no";

			return isYes;
		},
		false);
}

/*****************************************************************************/
std::string Output::getAnsiStyle(const Color inColor)
{
#if defined(CHALET_WIN32)
	if (Shell::isVisualStudioOutput())
		return std::string();
#endif

	if (inColor == Color::None)
		return std::string();

#if defined(CHALET_WIN32)
	bool isCmdPromptLike = Shell::isCommandPromptOrPowerShell();
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
#if defined(CHALET_WIN32)
	if (Shell::isVisualStudioOutput())
		return std::string();
#endif

	if (inColor == Color::None)
		return std::string();

#if defined(CHALET_WIN32)
	bool isCmdPromptLike = Shell::isCommandPromptOrPowerShell();
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
		const auto reset = getAnsiStyle(state.theme.reset);
		auto message = fmt::format("{}{}  {}{}\n", color, inSymbol, inMessage, reset);
		std::cout.write(message.data(), message.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::lineBreak(const bool inForce)
{
	if (!state.quietNonBuild || inForce)
	{
		auto reset = getAnsiStyle(state.theme.reset);
		std::cout.write(reset.data(), reset.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::lineBreakStderr()
{
	if (!state.quietNonBuild)
	{
		auto& errStream = getErrStream();
		auto reset = getAnsiStyle(state.theme.reset);
		errStream.write(reset.data(), reset.size());
		errStream.write("\n", 1);
		errStream.flush();
	}
}

/*****************************************************************************/
void Output::previousLine(const bool inForce)
{
	if ((!state.quietNonBuild || inForce))
	{
#if defined(CHALET_WIN32)
		if (!Shell::isVisualStudioOutput())
#endif
		{
			std::string eraser(80, ' ');
			auto prevLine = fmt::format("{}[F{}", getEscapeChar(), eraser);
			std::cout.write(prevLine.data(), prevLine.size());
			std::cout.put('\n');
			prevLine = fmt::format("{}[F", getEscapeChar());
			std::cout.write(prevLine.data(), prevLine.size());
			std::cout.flush();
		}
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto reset = getAnsiStyle(state.theme.reset);
		std::string output;
		if (inColor == Color::Reset)
		{
			output = fmt::format("{}{}", reset, inText);
		}
		else
		{
			const auto color = getAnsiStyle(inColor);
			output = fmt::format("{}{}{}", color, inText, reset);
		}

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto reset = getAnsiStyle(state.theme.reset);
		if (inColor == Color::Reset)
		{
			auto output = fmt::format("{}{}", reset, String::join(inList));

			std::cout.write(output.data(), output.size());
			std::cout.put('\n');
			std::cout.flush();
		}
		else
		{
			const auto color = getAnsiStyle(inColor);
			auto output = fmt::format("{}{}{}", color, String::join(inList), reset);

			std::cout.write(output.data(), output.size());
			std::cout.put('\n');
			std::cout.flush();
		}
	}
}

/*****************************************************************************/
void Output::printCommand(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.build);
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printCommand(const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.build);
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}", color, String::join(inList), reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printInfo(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.info);
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printFlair(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.flair);
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printSeparator(const char inChar)
{
	if (!state.quietNonBuild)
	{
		const auto color = getAnsiStyle(state.theme.flair);
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}", color, std::string(80, inChar), reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::msgFetchingDependency(const std::string& inPath)
{
	auto symbol = Unicode::heavyCurvedDownRightArrow();
	displayStyledSymbol(state.theme.note, symbol, fmt::format("Fetching: {}", inPath));
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
		const auto reset = getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}FAILED: {}{}", colorError, reset, inMessage);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	// Always display this (don't handle state.quietNonBuild)
	//
	auto symbol = Unicode::heavyBallotX();

	const auto color = getAnsiStyle(state.theme.error);
	const auto reset = getAnsiStyle(state.theme.reset);

	auto output = fmt::format("{}{}  Failed!\n   Review the errors above.{}", color, symbol, reset);

	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();
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
void Output::msgProfilerStartedSample(const std::string& inExecutable, const u32 inDuration, const u32 inSamplingInterval)
{
	Diagnostic::info("Sampling {} for {} seconds with {} millisecond of run time between samples", inExecutable, inDuration, inSamplingInterval);
}

/*****************************************************************************/
void Output::msgProfilerDone(const std::string& inProfileAnalysis)
{
	auto symbol = Unicode::diamond();
	displayStyledSymbol(state.theme.note, symbol, fmt::format("Profiler completed. View {} for details.", inProfileAnalysis));
}

/*****************************************************************************/
void Output::msgProfilerDoneAndLaunching(const std::string& inProfileAnalysis, const std::string& inApplication)
{
	auto symbol = Unicode::diamond();
	if (inApplication.empty())
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Profiler completed. Opening {}", inProfileAnalysis));
	else
		displayStyledSymbol(state.theme.note, symbol, fmt::format("Profiler completed. Launching {} in {}.", inProfileAnalysis, inApplication));
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
void Output::msgTargetOfType(const std::string& inLabel, const std::string& inName, const Color inColor)
{
	auto symbol = Unicode::triangle();
	displayStyledSymbol(inColor, symbol, fmt::format("{}: {}", inLabel, inName));
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
void Output::msgScanningForModuleDependencies()
{
	print(state.theme.build, "   Scanning sources for module dependencies...");
}

/*****************************************************************************/
void Output::msgBuildingRequiredHeaderUnits()
{
	print(state.theme.build, "   Building required header units...");
}

/*****************************************************************************/
void Output::msgModulesCompiling()
{
	print(state.theme.build, "   Compiling...");
}

/*****************************************************************************/
void Output::msgCopying(const std::string& inFrom, const std::string& inTo)
{
	return Output::msgAction(fmt::format("Copying: {}", inFrom), inTo);
}

/*****************************************************************************/
void Output::msgAction(const std::string& inLabel, const std::string& inTo)
{
	const auto reset = getAnsiStyle(state.theme.reset);
	const auto flair = getAnsiStyle(state.theme.flair);
	const auto build = getAnsiStyle(state.theme.build);

	Diagnostic::stepInfo("{}{} {}->{} {}", build, inLabel, flair, reset, inTo);
}
}