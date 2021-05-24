/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

#include <exception>

#include "Libraries/Format.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
namespace
{
/*****************************************************************************/
bool sExceptionThrown = false;
bool sAssertionFailure = false;
}

/*****************************************************************************/
const char* Diagnostic::CriticalException::what() const throw()
{
	return "A critical error occurred. Review output above";
}

/*****************************************************************************/
Diagnostic::CriticalException Diagnostic::kCriticalError;

/*****************************************************************************/
void Diagnostic::info(const std::string& inMessage, const bool inLineBreak)
{
	const auto color = Output::getAnsiStyle(Color::Black);
	const auto reset = Output::getAnsiReset();
	const auto symbol = '>';

	std::cout << fmt::format("{}{}  {}{}", color, symbol, reset, inMessage);
	if (inLineBreak)
		std::cout << std::endl;
	else
		std::cout << fmt::format("{} ... {}", color, reset) << std::flush;
}

/*****************************************************************************/
void Diagnostic::printDone(const std::string& inExtra)
{
	const auto color = Output::getAnsiStyle(Color::Black);
	const auto reset = Output::getAnsiReset();
	if (!inExtra.empty())
	{
		std::cout << fmt::format("{}done ({}){}", color, inExtra, reset) << std::endl;
	}
	else
	{
		std::cout << fmt::format("{}done{}", color, reset) << std::endl;
	}
}

/*****************************************************************************/
void Diagnostic::warn(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Warning;
	// Diagnostic::showHeader(type, inTitle);
	// Diagnostic::showMessage(type, inMessage);
	Diagnostic::showAsOneLine(type, inTitle, inMessage);

	// const auto reset = Output::getAnsiReset();
	// std::cout << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::error(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Error;
	// Diagnostic::showHeader(type, inTitle);
	// Diagnostic::showMessage(type, inMessage);
	Diagnostic::showAsOneLine(type, inTitle, inMessage);

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

	if (Environment::isBashOrWindowsConPTY())
	{
		const auto boldBlack = Output::getAnsiStyle(Color::Black, true);
		std::cerr << boldBlack;
	}

	sExceptionThrown = true;

	if (!inThrow)
		return;

	throw kCriticalError;
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber)
{
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

	std::abort();
}

/*****************************************************************************/
bool Diagnostic::assertionFailure() noexcept
{
	return sAssertionFailure;
}

/*****************************************************************************/
void Diagnostic::showHeader(const Type inType, const std::string& inTitle)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	const auto color = Output::getAnsiStyle(inType == Type::Error ? Color::Red : Color::Yellow, true);
	const auto reset = Output::getAnsiReset();

	out << color << inTitle << ':' << reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, const std::string& inMessage)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	out << fmt::format("   {}", inMessage) << std::endl;
}

/*****************************************************************************/
void Diagnostic::showAsOneLine(const Type inType, const std::string& inTitle, const std::string& inMessage)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	const auto color = Output::getAnsiStyle(inType == Type::Error ? Color::Red : Color::Yellow, true);
	const auto reset = Output::getAnsiReset();

	out << fmt::format("{}{}| {}{}", color, inTitle, reset, inMessage) << std::endl;
}

}