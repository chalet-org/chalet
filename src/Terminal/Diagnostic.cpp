/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

#include <exception>

#include "Libraries/Format.hpp"
#include "Libraries/Rang.hpp"

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
Constant Diagnostic::CriticalException::what() const throw()
{
	return "A critical error occurred. Review output above";
}

/*****************************************************************************/
Diagnostic::CriticalException Diagnostic::kCriticalError;

/*****************************************************************************/
void Diagnostic::warn(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Warning;
	Diagnostic::showHeader(type, inTitle);
	Diagnostic::showMessage(type, inMessage);

	std::cout << rang::style::reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::error(const std::string& inMessage, const std::string& inTitle)
{
	auto type = Type::Error;
	Diagnostic::showHeader(type, inTitle);
	Diagnostic::showMessage(type, inMessage);

	std::cerr << rang::style::reset << std::endl;
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

	// rang formatting gets used in the exception message
	std::cerr << rang::style::bold << rang::fg::black;
	sExceptionThrown = true;

	if (!inThrow)
		return;

	throw kCriticalError;
}

/*****************************************************************************/
void Diagnostic::customAssertion(const std::string_view& inExpression, const std::string_view& inMessage, const std::string_view& inFile, const uint inLineNumber)
{
	std::cerr << "\n"
			  << rang::style::bold << rang::fg::red << "Assertion Failed:\n  at "
			  << rang::style::reset << inExpression << " " << rang::fg::blue << inFile << ":" << inLineNumber << rang::style::reset << std::endl;

	if (!inMessage.empty())
		std::cerr << "\n"
				  << rang::style::bold << rang::fg::black << inMessage << rang::style::reset << std::endl;

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
	auto color = inType == Type::Error ? rang::fg::red : rang::fg::yellow;

	out << rang::style::bold << color << inTitle << ":" << rang::style::reset << std::endl;
}

/*****************************************************************************/
void Diagnostic::showMessage(const Type inType, const std::string& inMessage)
{
	auto& out = inType == Type::Error ? std::cerr : std::cout;
	out << fmt::format("   {}", inMessage) << std::endl;
}

}