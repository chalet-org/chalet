/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_CONDUCTOR_HPP
#define CHALET_COMMAND_CONDUCTOR_HPP

#include "Router/Route.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;
struct CentralState;

class Router
{
	using RouteAction = std::function<bool(Router&)>;
	using RouteList = std::unordered_map<Route, RouteAction>;

public:
	explicit Router(CommandLineInputs& inInputs);

	bool run();

private:
	bool runRoutesThatRequireState(const Route inRoute);

	bool routeConfigure(BuildState& inState);
	bool routeBundle(BuildState& inState);
	bool routeInit();
	bool routeSettings(const Route inRoute);
	bool routeQuery();
	bool routeTerminalTest();

	bool parseTheme();
	bool routeExport(CentralState& inCentralState);

#if defined(CHALET_DEBUG)
	bool routeDebug();
#endif

	CommandLineInputs& m_inputs;
};
}

#endif // CHALET_COMMAND_CONDUCTOR_HPP
