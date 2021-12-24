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
	Diagnostic::showInfo(fmt::format(std::move(inFmt), (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::infoEllipsis(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showInfo(fmt::format(std::move(inFmt), (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfo(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = true;
	Diagnostic::showStepInfo(fmt::format(std::move(inFmt), (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::stepInfoEllipsis(std::string_view inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showStepInfo(fmt::format(std::move(inFmt), (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::warn(std::string_view inFmt, Args&&... args)
{
	auto type = Type::Warning;
	Diagnostic::addError(type, fmt::format(std::move(inFmt), (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::error(std::string_view inFmt, Args&&... args)
{
	auto type = Type::Error;
	Diagnostic::addError(type, fmt::format(std::move(inFmt), (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::errorAbort(std::string_view inFmt, Args&&... args)
{
	Diagnostic::showErrorAndAbort(fmt::format(std::move(inFmt), (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename... Args>
void Diagnostic::fatalError(std::string_view inFmt, Args&&... args)
{
	Diagnostic::showFatalError(fmt::format(std::move(inFmt), (std::forward<Args>(args))...));
}
}
