/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

#include <exception>

#include "Libraries/Format.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/SignalHandler.hpp"

namespace chalet
{
/*****************************************************************************/
namespace
{
/*****************************************************************************/
bool sExceptionThrown = false;
bool sAssertionFailure = false;
bool sStartedInfo = false;
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
void Diagnostic::info(const std::string& inMessage, const bool inLineBreak)
{
	if (!Output::quietNonBuild())
	{
		const auto color = Output::getAnsiStyle(Color::Black);
		const auto reset = Output::getAnsiReset();
		const auto symbol = '>';

		std::cout << fmt::format("{}{}  {}{}", color, symbol, reset, inMessage);
		if (inLineBreak)
		{
			std::cout << std::endl;
		}
		else
		{
			std::cout << fmt::format("{} ... {}", color, reset) << std::flush;
			sStartedInfo = true;
		}
	}
}

/*****************************************************************************/
void Diagnostic::printDone(const std::string& inExtra)
{
	if (!Output::quietNonBuild())
	{
		const auto color = Output::getAnsiStyle(Color::Black);
		const auto reset = Output::getAnsiReset();

		sStartedInfo = false;
		if (!inExtra.empty())
		{
			std::cout << fmt::format("{}done ({}){}", color, inExtra, reset) << std::endl;
		}
		else
		{
			std::cout << fmt::format("{}done{}", color, reset) << std::endl;
		}
	}
}

/*****************************************************************************/
void Diagnostic::warn(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Warning;
	// Diagnostic::showHeader(type, inTitle);
	// Diagnostic::showMessage(type, inMessage);
	// Diagnostic::showAsOneLine(type, inTitle, inMessage);
	Diagnostic::addError(type, inMessage);
	UNUSED(inTitle);

	// const auto reset = Output::getAnsiReset();
	// std::cout << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::error(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Error;
	// Diagnostic::showHeader(type, inTitle);
	// Diagnostic::showMessage(type, inMessage);
	// Diagnostic::showAsOneLine(type, inTitle, inMessage);
	Diagnostic::addError(type, inMessage);
	UNUSED(inTitle);

	// const auto reset = Output::getAnsiReset();
	// std::cerr << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::warnHeader(const std::string& inTitle)
{
	Diagnostic::showHeader(Type::Warning, inTitle);
}

void Diagnostic::warnMessage(const std::string& inMessage)
{
	Diagnostic::showMessage(Type::Warning, inMessage);
}

/*****************************************************************************/
void Diagnostic::errorHeader(const std::string& inTitle)
{
	Diagnostic::showHeader(Type::Error, inTitle);
}

void Diagnostic::errorMessage(const std::string& inMessage)
{
	Diagnostic::showMessage(Type::Error, inMessage);
}

/*****************************************************************************/
void Diagnostic::errorAbort(const std::string& inMessage, const std::string& inTitle, const bool inThrow)
{
	if (sExceptionThrown)
		return;

	Diagnostic::error(inMessage, inTitle);
	Diagnostic::printErrors();

	if (Environment::isBashOrWindowsConPTY())
	{
		const auto boldBlack = Output::getAnsiStyle(Color::Black, true);
		std::cerr << boldBlack;
	}

	sExceptionThrown = true;

	if (!inThrow)
		return;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber)
{
	if (sStartedInfo)
	{
		std::cout << std::endl;
		sStartedInfo = false;
	}

	const auto boldRed = Output::getAnsiStyle(Color::Red, true);
	const auto boldBlack = Output::getAnsiStyle(Color::Black, true);
	const auto blue = Output::getAnsiStyle(Color::Blue);
	const auto reset = Output::getAnsiReset();

	std::cerr << '\n'
			  << boldRed << "Assertion Failed:\n  at " << reset
			  << inExpression << ' ' << blue << inFile << ':' << inLineNumber << reset << std::endl;

	if (!inMessage.empty())
		std::cerr << '\n'
				  << boldBlack << inMessage << reset << std::endl;

	sAssertionFailure = true;

	priv::SignalHandler::handler(SIGABRT);
}

/*****************************************************************************/
bool Diagnostic::assertionFailure() noexcept
{
	return sAssertionFailure;
}

/*****************************************************************************/
void Diagnostic::showHeader(const Type inType, const std::string& inTitle)
{
	if (sStartedInfo)
	{
		std::cout << std::endl;
		sStartedInfo = false;
	}

	auto& out = inType == Type::Error ? std::cerr : std::cout;
	const auto color = Output::getAnsiStyle(inType == Type::Error ? Color::Red : Color::Yellow, true);
	const auto reset = Output::getAnsiReset();

	out << color << inTitle << ':' << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, const std::string& inMessage)
{
	if (sStartedInfo)
	{
		std::cout << std::endl;
		sStartedInfo = false;
	}

	auto& out = inType == Type::Error ? std::cerr : std::cout;
	out << fmt::format("   {}", inMessage) << std::endl;
}

/*****************************************************************************/
void Diagnostic::showAsOneLine(const Type inType, const std::string& inTitle, const std::string& inMessage)
{
	if (sStartedInfo)
	{
		std::cout << std::endl;
		sStartedInfo = false;
	}

	auto& out = inType == Type::Error ? std::cerr : std::cout;
	const auto color = Output::getAnsiStyle(inType == Type::Error ? Color::Red : Color::Yellow, true);
	const auto reset = Output::getAnsiReset();

	out << fmt::format("{}{}| {}{}", color, inTitle, reset, inMessage) << std::endl;
}

/*****************************************************************************/
void Diagnostic::addError(const Type inType, const std::string& inMessage)
{
	getErrorList()->push_back({ inType, inMessage });
}

/*****************************************************************************/
void Diagnostic::printErrors()
{
	if (s_ErrorList != nullptr)
	{
		{
			auto& errorList = *s_ErrorList;
			if (errorList.size() > 0)
			{
				Diagnostic::errorHeader("Errors");

				for (auto& error : errorList)
				{
					if (error.message.empty())
						continue;

					Diagnostic::errorMessage(error.message);

					/*switch (error.type)
				{
					case JsonErrorClassification::Fatal:
						break;
					case JsonErrorClassification::Warning:
						// Output::print(Color::Yellow, fmt::format("    Attempting to continue build anyway..."));
						break;
					default:
						break;
				}*/
				}
				Output::lineBreak();
			}
		}

		delete s_ErrorList;
		s_ErrorList = nullptr;
	}

	s_Printed = true;
}

}