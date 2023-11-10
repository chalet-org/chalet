/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportKind.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;
struct CentralState;

class Router
{
public:
	explicit Router(CommandLineInputs& inInputs);

	bool run();

private:
	bool runRoutesThatRequireState();

	bool routeConfigure(BuildState& inState);
	bool routeBundle(BuildState& inState);
	bool routeInit();
	bool routeSettings();
	bool routeValidate();
	bool routeQuery();
	bool routeTerminalTest();

	bool routeExport(CentralState& inCentralState);

	bool parseTheme();
	bool workingDirectoryIsGlobalChaletDirectory();

#if defined(CHALET_DEBUG)
	bool routeDebug();
#endif

	CommandLineInputs& m_inputs;
};
}
