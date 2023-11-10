/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Core/Router/RouteType.hpp"

namespace chalet
{
struct CommandRoute
{
	constexpr CommandRoute() = default;
	explicit constexpr CommandRoute(const RouteType inValue);

	constexpr RouteType type() const noexcept;

	constexpr bool isUnknown() const noexcept;
	constexpr bool isHelp() const noexcept;
	constexpr bool isConfigure() const noexcept;
	constexpr bool isClean() const noexcept;
	constexpr bool isRebuild() const noexcept;
	constexpr bool isRun() const noexcept;
	constexpr bool isBuild() const noexcept;
	constexpr bool isBuildRun() const noexcept;
	constexpr bool isExport() const noexcept;
	constexpr bool isBundle() const noexcept;
	constexpr bool isQuery() const noexcept;
	constexpr bool isValidate() const noexcept;

	constexpr bool willRun() const noexcept;

private:
	RouteType m_route = RouteType::Unknown;
};
}

#include "Core/Router/CommandRoute.inl"
