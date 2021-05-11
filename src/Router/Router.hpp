/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_CONDUCTOR_HPP
#define CHALET_COMMAND_CONDUCTOR_HPP

#include "Router/Route.hpp"
#include "State/BuildState.hpp"
#include "State/CommandLineInputs.hpp"

namespace chalet
{
class Router
{
	using RouteAction = std::function<bool(Router&)>;
	using RouteList = std::unordered_map<Route, RouteAction>;

public:
	explicit Router(const CommandLineInputs& inInputs);
	~Router();

	bool run();

private:
	bool cmdConfigure();
	bool cmdBuild();
	bool cmdBundle();
	bool cmdInit();

#if defined(CHALET_DEBUG)
	bool cmdDebug();
#endif

	bool parseCacheJson();
	bool parseBuildJson(const std::string& inFile);
	void fetchToolVersions();
	bool installDependencies();
	bool xcodebuildRoute();

	bool managePathVariables();

	const CommandLineInputs& m_inputs;

	RouteList m_routes;

	std::unique_ptr<BuildState> m_buildState;
};
}

#endif // CHALET_COMMAND_CONDUCTOR_HPP
