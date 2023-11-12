/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/Diagnostic.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename... Args>
void Diagnostic::info(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = true;
	Diagnostic::showInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::infoEllipsis(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = false;
	Diagnostic::showInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::subInfo(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = true;
	Diagnostic::showSubInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::subInfoEllipsis(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = false;
	Diagnostic::showSubInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfo(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = true;
	Diagnostic::showStepInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfoEllipsis(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr bool lineBreak = false;
	Diagnostic::showStepInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::warn(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr auto type = Type::Warning;
	Diagnostic::addError(type, fmt::format(inFmt, (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::error(fmt::format_string<Args...> inFmt, Args&&... args)
{
	constexpr auto type = Type::Error;
	Diagnostic::addError(type, fmt::format(inFmt, (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::errorAbort(fmt::format_string<Args...> inFmt, Args&&... args)
{
	Diagnostic::showErrorAndAbort(fmt::format(inFmt, (std::forward<Args>(args))...));
}
}
