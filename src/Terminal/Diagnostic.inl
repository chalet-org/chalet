/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename... Args>
void Diagnostic::info(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = true;
	Diagnostic::showInfo(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::infoEllipsis(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showInfo(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfo(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = true;
	Diagnostic::showStepInfo(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfoEllipsis(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showStepInfo(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::warn(std::string_view inFmt, Args&&... args)
{
	auto type = Type::Warning;
	Diagnostic::addError(type, fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::error(std::string_view inFmt, Args&&... args)
{
	auto type = Type::Error;
	Diagnostic::addError(type, fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::errorAbort(std::string_view inFmt, Args&&... args)
{
	Diagnostic::showErrorAndAbort(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::fatalError(std::string_view inFmt, Args&&... args)
{
	Diagnostic::showFatalError(fmt::vformat(inFmt, fmt::make_args_checked<Args...>(inFmt, (std::forward<Args>(args))...)));
}
}
