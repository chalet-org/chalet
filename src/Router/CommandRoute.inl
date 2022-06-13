/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/CommandRoute.hpp"

namespace chalet
{
/*****************************************************************************/
constexpr CommandRoute::CommandRoute(const RouteType inValue) :
	m_route(inValue)
{
}

constexpr RouteType CommandRoute::type() const noexcept
{
	return m_route;
}

/*****************************************************************************/
constexpr bool CommandRoute::isUnknown() const noexcept
{
	return m_route == RouteType::Unknown;
}

/*****************************************************************************/
constexpr bool CommandRoute::isHelp() const noexcept
{
	return m_route == RouteType::Help;
}

/*****************************************************************************/
constexpr bool CommandRoute::isConfigure() const noexcept
{
	return m_route == RouteType::Configure;
}

/*****************************************************************************/
constexpr bool CommandRoute::isClean() const noexcept
{
	return m_route == RouteType::Clean;
}

/*****************************************************************************/
constexpr bool CommandRoute::isRebuild() const noexcept
{
	return m_route == RouteType::Rebuild;
}

/*****************************************************************************/
constexpr bool CommandRoute::isRun() const noexcept
{
	return m_route == RouteType::Run;
}

/*****************************************************************************/
constexpr bool CommandRoute::isBuildRun() const noexcept
{
	return m_route == RouteType::BuildRun;
}

/*****************************************************************************/
constexpr bool CommandRoute::isExport() const noexcept
{
	return m_route == RouteType::Export;
}

/*****************************************************************************/
constexpr bool CommandRoute::isBundle() const noexcept
{
	return m_route == RouteType::Bundle;
}

/*****************************************************************************/
constexpr bool CommandRoute::isQuery() const noexcept
{
	return m_route == RouteType::Query;
}

/*****************************************************************************/
constexpr bool CommandRoute::willRun() const noexcept
{
	return m_route == RouteType::BuildRun || m_route == RouteType::Run;
}
}
