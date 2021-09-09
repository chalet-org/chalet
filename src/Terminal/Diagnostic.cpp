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
/*****************************************************************************/
namespace
{
/*****************************************************************************/
std::unique_ptr<Spinner> s_spinnerThread;

bool sExceptionThrown = false;
bool sAssertionFailure = false;

void destroySpinnerThread()
{
	if (s_spinnerThread != nullptr)
	{
		s_spinnerThread->stop();
		s_spinnerThread.reset();
	}
}
}

/*****************************************************************************/
const char* Diagnostic::CriticalException::what() const throw()
{
	return "A critical error occurred. Review output above";
}

/*****************************************************************************/
Diagnostic::CriticalException Diagnostic::kCriticalError;
Diagnostic::ErrorList* Diagnostic::s_ErrorList = nullptr;
bool Diagnostic::s_Printed = false;

/*****************************************************************************/
Diagnostic::ErrorList* Diagnostic::getErrorList()
{
	chalet_assert(!s_Printed, "");

	if (s_ErrorList == nullptr)
	{
		s_ErrorList = new ErrorList();
	}

	return s_ErrorList;
}

/*****************************************************************************/
void Diagnostic::printDone(const std::string& inTime)
{
	if (!Output::quietNonBuild())
	{
		const auto color = Output::getAnsiStyle(Output::theme().flair);
		const auto reset = Output::getAnsiReset();

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
}

/*****************************************************************************/
void Diagnostic::showInfo(std::string&& inMessage, const bool inLineBreak)
{
	if (!Output::quietNonBuild())
	{
		auto& theme = Output::theme();
		const auto color = Output::getAnsiStyle(theme.flair);
		const auto infoColor = Output::getAnsiStyle(theme.info);
		const auto reset = Output::getAnsiReset();
		const auto symbol = '>';

		std::cout << fmt::format("{}{}  {}{}", color, symbol, infoColor, inMessage);
		if (inLineBreak)
		{
			std::cout << reset << std::endl;
		}
		else
		{
			// std::cout << fmt::format("{} ... {}", color, reset);
			std::cout << color + "     ";

			if (Output::showCommands())
			{
				std::cout << reset << std::endl;
			}
			else
			{
				std::cout << std::flush;
				destroySpinnerThread();
				s_spinnerThread = std::make_unique<Spinner>();
				s_spinnerThread->start();
			}
		}
	}
}

/*****************************************************************************/
void Diagnostic::showErrorAndAbort(std::string&& inMessage)
{
	if (sExceptionThrown)
		return;

	Diagnostic::showMessage(Type::Error, std::move(inMessage));
	Diagnostic::printErrors();

	if (Environment::isBashGenericColorTermOrWindowsTerminal())
	{
		const auto boldBlack = Output::getAnsiStyle(Output::theme().flair, true);
		std::cerr << boldBlack;
	}

	sExceptionThrown = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber)
{
	if (s_spinnerThread != nullptr)
	{
		std::cerr << std::endl;
		destroySpinnerThread();
	}

	const auto boldRed = Output::getAnsiStyle(Output::theme().error, true);
	const auto boldBlack = Output::getAnsiStyle(Output::theme().flair, true);
	const auto blue = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiReset();

	std::cerr << '\n'
			  << boldRed << "Assertion Failed:\n  at " << reset
			  << inExpression << ' ' << blue << inFile << ':' << inLineNumber << reset << std::endl;

	if (!inMessage.empty())
	{
		std::cerr << '\n'
				  << boldBlack << inMessage << reset << std::endl;
	}

	sAssertionFailure = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
bool Diagnostic::assertionFailure() noexcept
{
	return sAssertionFailure;
}

/*****************************************************************************/
void Diagnostic::showHeader(const Type inType, std::string&& inTitle)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	if (s_spinnerThread != nullptr)
	{
		out << std::endl;
		destroySpinnerThread();
	}

	const auto color = Output::getAnsiStyle(inType == Type::Error ? Output::theme().error : Output::theme().warning, true);
	const auto reset = Output::getAnsiReset();

	out << color << std::move(inTitle) << ':' << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, std::string&& inMessage)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	if (s_spinnerThread != nullptr)
	{
		out << std::endl;
		destroySpinnerThread();
	}

	out << fmt::format("   {}", std::move(inMessage)) << std::endl;
}

/*****************************************************************************/
void Diagnostic::addError(const Type inType, std::string&& inMessage)
{
	getErrorList()->push_back({ inType, std::move(inMessage) });
}

/*****************************************************************************/
void Diagnostic::printErrors()
{
	if (s_ErrorList != nullptr)
	{
		{
			destroySpinnerThread();

			auto& errorList = *s_ErrorList;
			StringList warnings;
			StringList errors;
			for (auto& err : errorList)
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

		delete s_ErrorList;
		s_ErrorList = nullptr;
	}

	s_Printed = true;
}

/*****************************************************************************/
void Diagnostic::throwCriticalError()
{
	destroySpinnerThread();
	CHALET_THROW(kCriticalError);
}

}