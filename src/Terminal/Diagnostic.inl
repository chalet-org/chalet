/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Terminal/Diagnostic.hpp"

namespace chalet
{
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
}
