/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_DIAGNOSTIC_HPP
#define CHALET_DIAGNOSTIC_HPP

#include "Utility/Types.hpp"

namespace chalet
{
struct Diagnostic
{
	enum class Type : ushort
	{
		Warning,
		Error,
		CriticalError,
	};

	Diagnostic() = delete;

	static void printDone(const std::string& inTime = std::string());

	template <typename... Args>
	static void info(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void infoEllipsis(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void stepInfo(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void stepInfoEllipsis(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void warn(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void error(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void errorAbort(fmt::format_string<Args...> inFmt, Args&&... args);

	template <typename... Args>
	static void fatalError(fmt::format_string<Args...> inFmt, Args&&... args);

	static void fatalErrorFromException(const char* inError);

	static void customAssertion(const std::string_view inExpression, const std::string_view inMessage, const std::string_view inFile, const uint inLineNumber);
	static bool assertionFailure() noexcept;

	static void printErrors();
	static void clearErrors();
	static void throwCriticalError();

private:
	static void showInfo(std::string&& inMessage, const bool inLineBreak);
	static void showStepInfo(std::string&& inMessage, const bool inLineBreak);
	static void showFatalError(std::string&& inMessage);
	static void showErrorAndAbort(std::string&& inMessage);
	static void showHeader(const Type inType, std::string&& inTitle);
	static void showMessage(const Type inType, std::string&& inMessage);
	static void addError(const Type inType, std::string&& inMessage);
};
}

#include "Terminal/Diagnostic.inl"

#ifdef NDEBUG
	#define chalet_assert(expr, message)
#else
	#define chalet_assert(expr, message) static_cast<void>((expr) || (chalet::Diagnostic::customAssertion(#expr, message, __FILE__, __LINE__), 0))
#endif

#endif // CHALET_DIAGNOSTIC_HPP
