/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

#include <csignal>
#include <exception>

#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Spinner.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/SignalHandler.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
struct Error
{
	Diagnostic::Type type;
	std::string message;
};

/*****************************************************************************/
static struct : std::exception
{
	const char* what() const throw() final
	{
		return "A critical error occurred. Review output above";
	}
} kCriticalError;

/*****************************************************************************/
static struct
{
	std::vector<Error> errorList;
	Unique<Spinner> spinnerThread;

	bool exceptionThrown = false;
	bool assertionFailure = false;
} state;

/*****************************************************************************/
bool destroySpinnerThread()
{
	if (state.spinnerThread == nullptr)
		return false;

	bool result = state.spinnerThread->stop();
	state.spinnerThread.reset();
	return result;
}
}

/*****************************************************************************/
void Diagnostic::printDone(const std::string& inTime)
{
	if (Output::quietNonBuild())
		return;

	const auto color = Output::getAnsiStyle(Output::theme().flair);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	destroySpinnerThread();

	std::string done;
	// if (Output::showCommands())
	// 	done = "... done";
	// else
	done = "done";

	std::string output;
	if (!inTime.empty() && Output::showBenchmarks())
	{
		output = fmt::format("{}{} ({}){}", color, done, inTime, reset);
	}
	else
	{
		output = fmt::format("{}{}{}", color, done, reset);
	}

	std::cout.write(output.data(), output.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();
}

/*****************************************************************************/
void Diagnostic::showInfo(std::string&& inMessage, const bool inLineBreak)
{
	if (Output::quietNonBuild())
		return;

	const auto& theme = Output::theme();
	const auto color = Output::getAnsiStyle(theme.flair);
	const auto infoColor = Output::getAnsiStyle(theme.info);
	const auto reset = Output::getAnsiStyle(Color::Reset);
	const auto symbol = '>';

	auto output = fmt::format("{}{}  {}{}", color, symbol, infoColor, inMessage);
	std::cout.write(output.data(), output.size());

	if (inLineBreak)
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.put(std::cout.widen('\n'));
		std::cout.flush();
	}
	else
	{
		if (Output::showCommands())
		{
			output = fmt::format("{} ... {}", color, reset);
			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}
		else
		{
			std::cout.write(color.data(), color.size());
			std::cout.flush();

			destroySpinnerThread();
			state.spinnerThread = std::make_unique<Spinner>();
			state.spinnerThread->start();
		}
	}
}

/*****************************************************************************/
void Diagnostic::showStepInfo(std::string&& inMessage, const bool inLineBreak)
{
	if (Output::quietNonBuild())
		return;

	const auto& theme = Output::theme();
	const auto color = Output::getAnsiStyle(theme.flair);
	const auto infoColor = Output::getAnsiStyle(theme.build);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	auto output = fmt::format("{}   {}{}", color, infoColor, inMessage);
	std::cout.write(output.data(), output.size());

	if (inLineBreak)
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.put(std::cout.widen('\n'));
		std::cout.flush();
	}
	else
	{
		if (Output::showCommands())
		{
			output = fmt::format("{} ... {}", color, reset);
			std::cout.write(output.data(), output.size());
			std::cout.flush();
		}
		else
		{
			std::cout.write(color.data(), color.size());
			std::cout.flush();

			destroySpinnerThread();
			state.spinnerThread = std::make_unique<Spinner>();
			state.spinnerThread->start();
		}
	}
}

/*****************************************************************************/
void Diagnostic::showFatalError(std::string&& inMessage)
{
	auto theme = Output::theme();
	if (!theme.isPreset())
		theme.error = Color::RedBold;

	const auto color = Output::getAnsiStyle(theme.error);
	const auto reset = Output::getAnsiStyle(Color::Reset);
	auto output = fmt::format("{}Error: {}{}", color, reset, inMessage);

	std::cout.write(output.data(), output.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();
}

/*****************************************************************************/
void Diagnostic::showErrorAndAbort(std::string&& inMessage)
{
	if (state.exceptionThrown)
		return;

	Diagnostic::showMessage(Type::Error, std::move(inMessage));
	Diagnostic::printErrors();

	if (Environment::isBashGenericColorTermOrWindowsTerminal())
	{
		const auto boldBlack = Output::getAnsiStyle(Output::theme().flair);
		std::cerr.write(boldBlack.data(), boldBlack.size());
	}

	state.exceptionThrown = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber)
{
	if (state.spinnerThread != nullptr)
	{
		std::cerr.put(std::cout.widen('\n'));
		std::cerr.flush();
		destroySpinnerThread();
	}

	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto boldBlack = Output::getAnsiStyle(Output::theme().flair);
	const auto blue = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	std::string output = fmt::format("\n{}Assertion Failed:\n  at {}{} {}{}:{}{}", boldRed, reset, inExpression, blue, inFile, inLineNumber, reset);

	std::cerr.write(output.data(), output.size());
	std::cerr.put(std::cout.widen('\n'));
	std::cerr.flush();

	if (!inMessage.empty())
	{
		output = fmt::format("\n{}{}{}", boldBlack, inMessage, reset);

		std::cerr.write(output.data(), output.size());
		std::cerr.put(std::cout.widen('\n'));
		std::cerr.flush();
	}

	state.assertionFailure = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
bool Diagnostic::assertionFailure() noexcept
{
	return state.assertionFailure;
}

/*****************************************************************************/
void Diagnostic::showHeader(const Type inType, std::string&& inTitle)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	if (state.spinnerThread != nullptr)
	{
		out << std::endl;
		destroySpinnerThread();
	}

	const auto color = Output::getAnsiStyle(inType == Type::Error ? Output::theme().error : Output::theme().warning);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	out << color << std::move(inTitle) << ':' << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, std::string&& inMessage)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	if (state.spinnerThread != nullptr)
	{
		out << std::endl;
		destroySpinnerThread();
	}

	out << fmt::format("   {}", std::move(inMessage)) << std::endl;
}

/*****************************************************************************/
void Diagnostic::addError(const Type inType, std::string&& inMessage)
{
	state.errorList.push_back({ inType, std::move(inMessage) });
}

/*****************************************************************************/
void Diagnostic::printErrors()
{
	if (state.errorList.empty())
		return;

	if (state.spinnerThread != nullptr && !destroySpinnerThread())
	{
		if (!Environment::isSubprocess())
		{
			std::cout.put(std::cout.widen('\n'));
			std::cout.flush();
		}
	}

	StringList warnings;
	StringList errors;
	std::reverse(state.errorList.begin(), state.errorList.end());

	for (auto& err : state.errorList)
	{
		if (err.message.empty())
			continue;

		if (err.type == Type::Warning)
			warnings.emplace_back(std::move(err.message));
		else
			errors.emplace_back(std::move(err.message));
	}

	bool hasWarnings = false;
	if (warnings.size() > 0)
	{
		Type type = Type::Warning;
		Output::lineBreak();

		// Diagnostic::showHeader(type, fmt::format("{}  Warnings", Unicode::warning()));
		Diagnostic::showHeader(type, warnings.size() == 1 ? "Warning" : "Warnings");

		for (auto& message : warnings)
		{
			Diagnostic::showMessage(type, std::move(message));
		}

		if (errors.size() == 0)
			Output::lineBreak();

		hasWarnings = true;
	}

	if (errors.size() > 0)
	{
		Type type = Type::Error;
		if (!hasWarnings)
			Output::lineBreakStderr();

		Diagnostic::showHeader(type, errors.size() == 1 ? "Error" : "Errors");

		for (auto& message : errors)
		{
			Diagnostic::showMessage(type, std::move(message));
		}

		Output::lineBreak();
	}
}

/*****************************************************************************/
void Diagnostic::clearErrors()
{
	state.errorList.clear();
}

/*****************************************************************************/
void Diagnostic::throwCriticalError()
{
	destroySpinnerThread();
	CHALET_THROW(kCriticalError);
}

}