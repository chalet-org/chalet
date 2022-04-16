/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

namespace chalet
{
/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::info(const S& inFmt, Args&&... args)
{
	bool lineBreak = true;
	Diagnostic::showInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::infoEllipsis(const S& inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::stepInfo(const S& inFmt, Args&&... args)
{
	bool lineBreak = true;
	Diagnostic::showStepInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::stepInfoEllipsis(const S& inFmt, Args&&... args)
{
	bool lineBreak = false;
	Diagnostic::showStepInfo(fmt::format(inFmt, (std::forward<Args>(args))...), lineBreak);
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::warn(const S& inFmt, Args&&... args)
{
	auto type = Type::Warning;
	Diagnostic::addError(type, fmt::format(inFmt, (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::error(const S& inFmt, Args&&... args)
{
	auto type = Type::Error;
	Diagnostic::addError(type, fmt::format(inFmt, (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::errorAbort(const S& inFmt, Args&&... args)
{
	Diagnostic::showErrorAndAbort(fmt::format(inFmt, (std::forward<Args>(args))...));
}

/*****************************************************************************/
template <typename S, typename... Args>
void Diagnostic::fatalError(const S& inFmt, Args&&... args)
{
	Diagnostic::showFatalError(fmt::format(inFmt, (std::forward<Args>(args))...));
}
}
