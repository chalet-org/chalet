/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DIAGNOSTIC_HPP
#define CHALET_DIAGNOSTIC_HPP

#include "Utility/Types.hpp"

namespace chalet
{
const std::string kDefaultWarning = u8"\u26A0  Warning";
const std::string kDefaultError = "Error";

struct Diagnostic
{
	Diagnostic() = delete;

	static void warn(const std::string& inMessage, const std::string& inTitle = kDefaultWarning);
	static void error(const std::string& inMessage, const std::string& inTitle = kDefaultError);

	static void warnHeader(const std::string& inTitle = kDefaultWarning);
	static void warnMessage(const std::string& inMessage);

	static void errorHeader(const std::string& inTitle = kDefaultError);
	static void errorMessage(const std::string& inMessage);

	static void errorAbort(const std::string& inMessage, const std::string& inTitle = "Critical Error", const bool inThrow = true);

	static void customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber);
	static bool assertionFailure() noexcept;

private:
	enum class Type : ushort
	{
		Warning,
		Error
	};

	struct CriticalException : std::exception
	{
		const char* what() const throw() final;
	};

	static CriticalException kCriticalError;

	static void showHeader(const Type inType, const std::string& inTitle);
	static void showMessage(const Type inType, const std::string& inMessage);
	static void showAsOneLine(const Type inType, const std::string& inTitle, const std::string& inMessage);
};
}

#ifdef NDEBUG
	#define chalet_assert(expr, message)
#else
	#define chalet_assert(expr, message) static_cast<void>((expr) || (chalet::Diagnostic::customAssertion(#expr, message, __FILE__, __LINE__), 0))
#endif

#endif // CHALET_DIAGNOSTIC_HPP
