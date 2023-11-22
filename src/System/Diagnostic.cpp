/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/Diagnostic.hpp"

#include <csignal>
#include <exception>

#include "System/SignalHandler.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Terminal/Spinner.hpp"
#include "Terminal/Unicode.hpp"
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
struct : std::exception
{
	const char* what() const throw() final
	{
		return "A critical error occurred. Review output above";
	}
} kCriticalError;

/*****************************************************************************/
struct
{
	std::vector<Error> errorList;

	bool padded = false;
	bool exceptionThrown = false;
	bool assertionFailure = false;
} state;

/*****************************************************************************/
bool destroySpinnerThread(const bool cancel = false)
{
	if (!Spinner::instanceCreated())
		return true;

	bool result = false;
	if (cancel)
		result = Spinner::instance().cancel();
	else
		result = Spinner::instance().stop();

	if (result)
	{
		Spinner::destroyInstance();
	}

	return result;
}
}

/*****************************************************************************/
void Diagnostic::cancelEllipsis()
{
	if (Output::quietNonBuild())
		return;

	if (!Spinner::instanceCreated())
		return;

	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	if (destroySpinnerThread(true))
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Diagnostic::printDone(const std::string& inTime)
{
	if (Output::quietNonBuild())
		return;

	const auto color = Output::getAnsiStyle(Output::theme().flair);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	if (destroySpinnerThread())
	{
		auto word{ "done" };
		std::string output;
		if (!inTime.empty() && Output::showBenchmarks())
		{
			output = fmt::format("{}{} ({}){}", color, word, inTime, reset);
		}
		else
		{
			output = fmt::format("{}{}{}", color, word, reset);
		}

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Diagnostic::printValid(const bool inValid)
{
	if (Output::quietNonBuild())
		return;

	const auto color = Output::getAnsiStyle(inValid ? Output::theme().flair : Output::theme().error);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	if (destroySpinnerThread())
	{
		auto valid = inValid ? "valid" : "FAILED";
		auto output = fmt::format("{}{}{}{}", reset, color, valid, reset);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}
}

/*****************************************************************************/
void Diagnostic::printFound(const bool inFound, const std::string& inTime)
{
	if (Output::quietNonBuild())
		return;

	const auto color = Output::getAnsiStyle(inFound ? Output::theme().flair : Output::theme().error);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	if (destroySpinnerThread())
	{
		auto words = inFound ? "found" : "not found";
		std::string output;
		if (!inTime.empty() && Output::showBenchmarks())
		{
			output = fmt::format("{}{}{} ({}){}", reset, color, words, inTime, reset);
		}
		else
		{
			output = fmt::format("{}{}{}{}", reset, color, words, reset);
		}

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
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
	const auto reset = Output::getAnsiStyle(theme.reset);
	const auto symbol = '>';

	auto output = fmt::format("{}{}  {}{}", color, symbol, infoColor, inMessage);
	std::cout.write(output.data(), output.size());

	if (inLineBreak)
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.put('\n');
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
			Spinner::instance().start();
		}
	}
}

/*****************************************************************************/
// Note: don't use the Spinner here - would slow down operations like the
//   batch validator
//
void Diagnostic::showSubInfo(std::string&& inMessage, const bool inLineBreak)
{
	if (Output::quietNonBuild())
		return;

	const auto& theme = Output::theme();
	const auto color = Output::getAnsiStyle(theme.flair);
	const auto infoColor = Output::getAnsiStyle(theme.info);
	const auto reset = Output::getAnsiStyle(theme.reset);

	auto output = fmt::format("{}   + {}{}", color, infoColor, inMessage);
	std::cout.write(output.data(), output.size());

	if (inLineBreak)
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.put('\n');
		std::cout.flush();
	}
	else
	{
		output = fmt::format("{} -- {}", color, reset);
		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}
}

/*****************************************************************************/
void Diagnostic::showStepInfo(std::string&& inMessage, const bool inLineBreak)
{
	if (Output::quietNonBuild())
		return;

	const auto color = Output::getAnsiStyle(Output::theme().flair);
	const auto infoColor = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	auto output = fmt::format("{}   {}{}", color, infoColor, inMessage);
	std::cout.write(output.data(), output.size());

	if (inLineBreak)
	{
		std::cout.write(reset.data(), reset.size());
		std::cout.put('\n');
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
			Spinner::instance().start();
		}
	}
}

/*****************************************************************************/
void Diagnostic::showErrorAndAbort(std::string&& inMessage)
{
	if (state.exceptionThrown)
		return;

	Diagnostic::addError(Type::Error, std::move(inMessage));
	Diagnostic::printErrors();

	if (Shell::isBashGenericColorTermOrWindowsTerminal())
	{
		const auto boldBlack = Output::getAnsiStyle(Output::theme().flair);
		Output::getErrStream().write(boldBlack.data(), boldBlack.size());
	}

	state.exceptionThrown = true;

	::raise(SIGABRT);
}

/*****************************************************************************/
void Diagnostic::fatalErrorFromException(const char* inError)
{
	Diagnostic::addError(Type::Error, std::string(inError));
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const u32 inLineNumber)
{
	auto& errStream = Output::getErrStream();
	if (Spinner::instanceCreated())
	{
		errStream.put('\n');
		errStream.flush();
		destroySpinnerThread();
	}

	const auto boldRed = Output::getAnsiStyle(Output::theme().error);
	const auto boldBlack = Output::getAnsiStyle(Output::theme().flair);
	const auto blue = Output::getAnsiStyle(Output::theme().build);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	std::string output = fmt::format("\n{}Assertion Failed:\n  at {}{} {}{}:{}{}", boldRed, reset, inExpression, blue, inFile, inLineNumber, reset);

	errStream.write(output.data(), output.size());
	errStream.put('\n');
	errStream.flush();

	if (!inMessage.empty())
	{
		output = fmt::format("\n{}{}{}", boldBlack, inMessage, reset);

		errStream.write(output.data(), output.size());
		errStream.put('\n');
		errStream.flush();
	}

	state.assertionFailure = true;

	::raise(SIGABRT);
}

/*****************************************************************************/
bool Diagnostic::assertionFailure() noexcept
{
	return state.assertionFailure;
}

/*****************************************************************************/
void Diagnostic::showHeader(const Type inType, std::string&& inTitle)
{
	auto& out = inType == Type::Error ? Output::getErrStream() : std::cout;
	if (Spinner::instanceCreated())
	{
		out << std::endl;
		destroySpinnerThread();
	}

	const auto color = Output::getAnsiStyle(inType == Type::Error || inType == Type::ErrorStdOut ? Output::theme().error : Output::theme().warning);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	out << fmt::format("{}{}: {}", color, inTitle, reset);
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, std::string&& inMessage)
{
	auto& out = inType == Type::Error ? Output::getErrStream() : std::cout;
	if (Spinner::instanceCreated())
	{
		out << std::endl;
		destroySpinnerThread();
	}

	out << inMessage << std::endl;
}

/*****************************************************************************/
void Diagnostic::addError(const Type inType, std::string&& inMessage)
{
	state.errorList.push_back({ inType, std::move(inMessage) });
}

/*****************************************************************************/
void Diagnostic::printErrors(const bool inForceStdOut)
{
	if (state.errorList.empty())
		return;

	if (Spinner::instanceCreated() && !destroySpinnerThread())
	{
		if (!Shell::isSubprocess())
		{
			std::cout.put('\n');
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

	const auto reset = Output::getAnsiStyle(Output::theme().reset);
	std::cout.write(reset.data(), reset.size());

	bool hasWarnings = false;
	if (!warnings.empty())
	{
		Output::setQuietNonBuild(false);

		Type type = Type::Warning;
		Output::lineBreak();

		{
			auto label = "WARNING";
			for (auto& warn : warnings)
			{
				Diagnostic::showHeader(type, label);
				Diagnostic::showMessage(type, std::move(warn));
			}
			warnings.clear();
		}

		if (errors.empty())
			Output::lineBreak();

		hasWarnings = true;
	}

	if (!errors.empty())
	{
		Output::setQuietNonBuild(false);

		Type type = inForceStdOut ? Type::ErrorStdOut : Type::Error;
		if (!hasWarnings && state.padded)
		{
			Output::lineBreakStderr();
		}

		{
			auto label = "ERROR";
			for (auto& err : errors)
			{
				Diagnostic::showHeader(type, label);
				Diagnostic::showMessage(type, std::move(err));
			}
			errors.clear();
		}

		if (state.padded)
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

/*****************************************************************************/
void Diagnostic::usePaddedErrors()
{
	state.padded = true;
}

}