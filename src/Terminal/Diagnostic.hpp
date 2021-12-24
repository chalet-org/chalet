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
		Error
	};

	Diagnostic() = delete;

	static void printDone(const std::string& inTime = std::string());

	template <typename... Args>
	static void info(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void infoEllipsis(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void stepInfo(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void stepInfoEllipsis(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void warn(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void error(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void errorAbort(std::string_view inFmt, Args&&... args);

	template <typename... Args>
	static void fatalError(std::string_view inFmt, Args&&... args);

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
