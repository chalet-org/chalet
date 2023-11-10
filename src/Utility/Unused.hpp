/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#define UNUSED(...) chalet::priv::unused(__VA_ARGS__)

namespace chalet::priv
{
template <typename... Args>
constexpr void unused(Args&&... args)
{
	(static_cast<void>(std::forward<Args>(args)), ...);
}
}
