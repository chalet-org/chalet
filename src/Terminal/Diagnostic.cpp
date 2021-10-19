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
	std::unique_ptr<Spinner> spinnerThread;

	bool exceptionThrown = false;
	bool assertionFailure = false;
} state;

/*****************************************************************************/
void destroySpinnerThread()
{
	if (state.spinnerThread == nullptr)
		return;

	state.spinnerThread->stop();
	state.spinnerThread.reset();
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
	if (Output::showCommands())
		done = "... done";
	else
		done = "done";

	if (!inTime.empty() && Output::showBenchmarks())
	{
		std::cout << fmt::format("{}{} ({}){}", color, done, inTime, reset) << std::endl;
	}
	else
	{
		std::cout << fmt::format("{}{}{}", color, done, reset) << std::endl;
	}
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

	std::cout << fmt::format("{}{}  {}{}", color, symbol, infoColor, inMessage);
	if (inLineBreak)
	{
		std::cout << reset << std::endl;
	}
	else
	{
		// std::cout << fmt::format("{} ... {}", color, reset);
		std::cout << color;

		if (Output::showCommands())
		{
			std::cout << reset << std::endl;
		}
		else
		{
			std::cout << std::flush;
			destroySpinnerThread();
			state.spinnerThread = std::make_unique<Spinner>();
			state.spinnerThread->start();
		}
	}
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
		std::cerr << boldBlack;
	}

	state.exceptionThrown = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber)
{
	if (state.spinnerThread != nullptr)
	{
		std::cerr << std::endl;
		destroySpinnerThread();
	}

	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto boldBlack = Output::getAnsiStyle(Output::theme().flair);
	const auto blue = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiStyle(Color::Reset);

	std::cerr << '\n'
			  << boldRed << "Assertion Failed:\n  at " << reset
			  << inExpression << ' ' << blue << inFile << ':' << inLineNumber << reset << std::endl;

	if (!inMessage.empty())
	{
		std::cerr << '\n'
				  << boldBlack << inMessage << reset << std::endl;
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

	destroySpinnerThread();

	StringList warnings;
	StringList errors;
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
		Diagnostic::showHeader(type, fmt::format("{}  Warnings", Unicode::warning()));

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

		Diagnostic::showHeader(type, fmt::format("{}  Errors", Unicode::circledSaltire()));

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