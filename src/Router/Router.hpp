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
struct StatePrototype;

class Router
{
	using RouteAction = std::function<bool(Router&)>;
	using RouteList = std::unordered_map<Route, RouteAction>;

public:
	explicit Router(const CommandLineInputs& inInputs);

	bool run();

private:
	bool cmdConfigure();
	bool cmdBundle(StatePrototype& inPrototype);
	bool cmdInit();

#if defined(CHALET_DEBUG)
	bool cmdDebug();
#endif

	bool parseEnvFile();
	bool xcodebuildRoute(BuildState& inState);

	bool managePathVariables(const StatePrototype* inPrototype);

	const CommandLineInputs& m_inputs;

	bool m_installDependencies = false;
};
}

#endif // CHALET_COMMAND_CONDUCTOR_HPP
