/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Output.hpp"

#include "Libraries/WindowsApi.hpp"
#include "Process/Environment.hpp"
#include "System/Files.hpp"
#include "System/SignalHandler.hpp"
#include "Terminal/Shell.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/StringWinApi.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
static struct
{
	std::string kEmpty;
	std::unordered_map<Color, std::string> colorCache;

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
void signalHandler(i32)
{
	Output::lineBreak();
	Output::lineBreak();

#if defined(CHALET_WIN32)
	throw std::runtime_error("SIGINT");
#else
	std::exit(1);
#endif
}

/*****************************************************************************/
constexpr char getEscapeChar()
{
	return '\x1b';
}

std::string getAnsiStyleInternal(const Color inColor)
{
	if (inColor == Color::None || Shell::isBasicOutput())
		return std::string();

#if defined(CHALET_WIN32)
	bool isCmdPromptLike = Shell::isCommandPromptOrPowerShell();
	if (isCmdPromptLike && !Output::ansiColorsSupportedInComSpec())
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
#if defined(CHALET_DEBUG)
	UNUSED(inValue);
#else
	state.quietNonBuild = inValue;
#endif
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
#if defined(CHALET_DEBUG)
	UNUSED(inValue);
#else
	state.allowCommandsToShow = inValue;
#endif
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
	if (Shell::isBasicOutput())
		return std::cout;

	return std::cerr;
}

/*****************************************************************************/
bool Output::getUserInput(const std::string& inUserQuery, std::string& outResult, std::string note, const std::function<bool(std::string&)>& onValidate, const bool inFailOnFalse)
{
	const auto& color = Output::getAnsiStyle(state.theme.flair);
	const auto& noteColor = Output::getAnsiStyle(state.theme.build);
	const auto& answerColor = Output::getAnsiStyle(state.theme.note);
	const auto& reset = Output::getAnsiStyle(state.theme.reset);
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
#if defined(CHALET_WIN32)
	{
		HANDLE kInputHandle = ::GetStdHandle(STD_INPUT_HANDLE);

		DWORD bufferSize = 256;
		TCHAR buffer[256];
		DWORD numberCharactersRead = 0;
		auto readResult = ::ReadConsole(kInputHandle, buffer, bufferSize, &numberCharactersRead, NULL);
		if (readResult == 0)
		{
			// cin clear?
			return getUserInput(inUserQuery, outResult, std::move(note), onValidate, inFailOnFalse);
		}

		auto result = TSTRING(buffer, static_cast<size_t>(numberCharactersRead));
		input = FROM_WIDE(result);
		if (String::endsWith("\r\n", input))
		{
			input.pop_back();
			input.pop_back();
		}
		else if (readResult == 1)
		{
			signalHandler(SIGINT);
		}
	}
#else
	SignalHandler::add(SIGINT, signalHandler);

	std::getline(std::cin, input); // get up to first line break (if applicable)

	SignalHandler::remove(SIGINT, signalHandler);

	if (std::cin.fail() || std::cin.eof())
	{
		std::cin.clear();
		return getUserInput(inUserQuery, outResult, std::move(note), onValidate, inFailOnFalse);
	}
#endif

	if (input.empty())
		input = outResult;

	bool result = onValidate(input);
	if (!result && inFailOnFalse)
	{
		const auto& error = Output::getAnsiStyle(state.theme.error);
		auto invalid = fmt::format("{cleanLine}{lineUp}{output}{input}{color} -- {error}invalid entry{reset}\n",
			FMT_ARG(cleanLine),
			FMT_ARG(lineUp),
			FMT_ARG(output),
			FMT_ARG(input),
			FMT_ARG(color),
			FMT_ARG(error),
			FMT_ARG(reset));

		std::cout.write(invalid.data(), invalid.size());
		std::cout.flush();

		return getUserInput(inUserQuery, outResult, std::move(note), onValidate, inFailOnFalse);
	}
	else
	{
		outResult = input;

		auto toOutput = fmt::format("{cleanLine}{lineUp}{output}{outResult}{reset}\n",
			FMT_ARG(cleanLine),
			FMT_ARG(lineUp),
			FMT_ARG(output),
			FMT_ARG(outResult),
			FMT_ARG(reset));

		// toOutput.append(80 - toOutput.size(), ' ');
		std::cout.write(toOutput.data(), toOutput.size());
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
const std::string& Output::getAnsiStyle(const Color inColor)
{
	if (state.colorCache.find(inColor) == state.colorCache.end())
		state.colorCache.emplace(inColor, getAnsiStyleInternal(inColor));

	return state.colorCache.at(inColor);
}

/*****************************************************************************/
std::string Output::getAnsiStyleRaw(const Color inColor)
{
	if (inColor == Color::None || Shell::isBasicOutput())
		return std::string();

#if defined(CHALET_WIN32)
	if (Shell::isCommandPromptOrPowerShell() && !ansiColorsSupportedInComSpec())
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
void Output::displayStyledSymbol(const Color inColor, const std::string_view inSymbol, const std::string& inMessage)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(inColor);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
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
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
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
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
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
		if (!Shell::isBasicOutput())
		{
			std::string eraser(80, ' ');
			auto prevLine = fmt::format("{}[F{}\n", getEscapeChar(), eraser);
			std::cout.write(prevLine.data(), prevLine.size());
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
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		std::string output;
		if (inColor == Color::Reset)
		{
			output = fmt::format("{}{}\n", reset, inText);
		}
		else
		{
			const auto& color = Output::getAnsiStyle(inColor);
			output = fmt::format("{}{}{}\n", color, inText, reset);
		}

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::print(const Color inColor, const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		if (inColor == Color::Reset)
		{
			auto output = fmt::format("{}{}\n", reset, String::join(inList));

			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}
		else
		{
			const auto& color = Output::getAnsiStyle(inColor);
			auto output = fmt::format("{}{}{}\n", color, String::join(inList), reset);

			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}
	}
}

/*****************************************************************************/
void Output::printCommand(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.build);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}\n", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printCommand(const StringList& inList)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.build);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}\n", color, String::join(inList), reset);

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printInfo(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.info);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}\n", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printFlair(const std::string& inText)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.flair);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}\n", color, inText, reset);

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::printSeparator(const char inChar)
{
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.flair);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}{}{}\n", color, std::string(80, inChar), reset);

		std::cout.write(output.data(), output.size());
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
	if (!state.quietNonBuild)
	{
		const auto& color = Output::getAnsiStyle(state.theme.success);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto symbol = Unicode::checkmark();
		auto message = fmt::format("{}{}  Succeeded!{}\n", color, symbol, reset);
		std::cout.write(message.data(), message.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::msgTargetUpToDate(const std::string& inContext, Timer* outTimer)
{
	if (!state.quietNonBuild)
	{
		i64 result = outTimer != nullptr ? outTimer->stop() : 0;
		std::string output;
		bool benchmark = result > 0 && Output::showBenchmarks();
		// const auto& reset = Output::getAnsiStyle(state.theme.reset);
		if (!benchmark || outTimer == nullptr)
		{
			output = fmt::format("   {}: done\n", inContext);
		}
		else
		{
			output = fmt::format("   {}: {}\n", inContext, outTimer->asString());
		}
		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::msgCommandPoolError(const std::string& inMessage)
{
	if (!state.quietNonBuild)
	{
		const auto& colorError = Output::getAnsiStyle(state.theme.error);
		const auto& reset = Output::getAnsiStyle(state.theme.reset);
		auto output = fmt::format("{}FAILED: {}{}\n", colorError, reset, inMessage);

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Output::msgBuildFail()
{
	// Always display this (don't handle state.quietNonBuild)
	//
	auto symbol = Unicode::heavyBallotX();

	const auto& color = Output::getAnsiStyle(state.theme.error);
	const auto& reset = Output::getAnsiStyle(state.theme.reset);

	auto output = fmt::format("{}{}  Failed!\n   Review the errors above.{}\n", color, symbol, reset);

	std::cout.write(output.data(), output.size());
	std::cout.flush();
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
	const char* label = inBuildConfiguration.empty() ? "All" : inBuildConfiguration.c_str();
	auto symbol = Unicode::triangle();
	displayStyledSymbol(state.theme.header, symbol, fmt::format("Clean: {}", label));
}

/*****************************************************************************/
void Output::msgTargetOfType(const char* inLabel, const std::string& inName, const Color inColor)
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
	const auto& reset = Output::getAnsiStyle(state.theme.reset);
	const auto& flair = Output::getAnsiStyle(state.theme.flair);
	const auto& build = Output::getAnsiStyle(state.theme.build);

	Diagnostic::stepInfo("{}{}{} -> {}{}", build, inLabel, flair, reset, inTo);
}

/*****************************************************************************/
void Output::msgActionEllipsis(const std::string& inLabel, const std::string& inTo)
{
	const auto& reset = Output::getAnsiStyle(state.theme.reset);
	const auto& flair = Output::getAnsiStyle(state.theme.flair);
	const auto& build = Output::getAnsiStyle(state.theme.build);

	Diagnostic::stepInfoEllipsis("{}{}{} -> {}{}", build, inLabel, flair, reset, inTo);
}
}