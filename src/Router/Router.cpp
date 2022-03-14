/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/QueryController.hpp"
#include "Export/IProjectExporter.hpp"
#include "Process/ProcessController.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/TerminalTest.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

#include "Libraries/Json.hpp"

#include <chrono>
#include <thread>

namespace chalet
{
/*****************************************************************************/
Router::Router(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool Router::run()
{
	if (!parseTheme())
		return false;

	Route route = m_inputs.route();
	if (route == Route::Unknown
		|| static_cast<std::underlying_type_t<Route>>(route) >= static_cast<std::underlying_type_t<Route>>(Route::Count))
	{
		Diagnostic::fatalError("Command not recognized.");
		return false;
	}

	// Routes that don't require state
	switch (route)
	{
		case Route::Query:
			return routeQuery();

		case Route::TerminalTest:
			return routeTerminalTest();

		case Route::Init:
			return routeInit();

		case Route::SettingsGet:
		case Route::SettingsSet:
		case Route::SettingsUnset:
		case Route::SettingsGetKeys:
			return routeSettings(route);

#if defined(CHALET_DEBUG)
		case Route::Debug:
			return routeDebug();
#endif

		default: break;
	}

	return runRoutesThatRequireState(route);
}

/*****************************************************************************/
bool Router::runRoutesThatRequireState(const Route inRoute)
{
	if (inRoute == Route::Export && m_inputs.exportKind() == ExportKind::None)
	{
		Diagnostic::fatalError("The requested project kind '{}' was not recognized, or is not yet supported.", m_inputs.exportKindRaw());
		return false;
	}

	auto centralState = std::make_unique<CentralState>(m_inputs);
	Unique<BuildState> buildState;

	if (!centralState->initialize())
		return false;

	if (inRoute != Route::Bundle && inRoute != Route::Export)
	{
		buildState = std::make_unique<BuildState>(centralState->inputs(), *centralState);
		if (!buildState->initialize())
			return false;
	}

	bool result = false;
	switch (inRoute)
	{
		case Route::Bundle: {
			result = routeBundle(*centralState);
			break;
		}

		case Route::Configure: {
			chalet_assert(buildState != nullptr, "");
			result = routeConfigure(*buildState);
			break;
		}

		case Route::BuildRun:
		case Route::Build:
		case Route::Rebuild:
		case Route::Run:
		case Route::Clean: {
			chalet_assert(buildState != nullptr, "");
			result = buildState->doBuild();
			break;
		}

		case Route::Export: {
			result = routeExport(*centralState);
			break;
		}

		default:
			break;
	}

	if (centralState != nullptr)
		centralState->saveCaches();

	return result;
}

/*****************************************************************************/
bool Router::routeConfigure(BuildState& inState)
{
	bool addLineBreak = false;
	if (inState.environment != nullptr)
	{
		addLineBreak |= inState.environment->ouptuttedDescription();
	}

	if (addLineBreak)
		Output::lineBreak();

	Output::msgConfigureCompleted(inState.workspace.workspaceName());
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::routeBundle(CentralState& inCentralState)
{
	const auto& inputFile = m_inputs.inputFile();
	if (inCentralState.distribution.empty())
	{
		Diagnostic::error("{}: There are no distribution targets: missing 'distribution'", inputFile);
		return false;
	}

	/*if (inCentralState.requiredBuildConfigurations().empty())
	{
		Diagnostic::error("{}: 'bundle' ran without any valid distribution bundles: missing 'configuration'", inputFile);
		return false;
	}*/

	AppBundler bundler(inCentralState);

	if (!bundler.runBuilds())
		return false;

	for (auto& target : inCentralState.distribution)
	{
		if (!bundler.run(target))
			return false;
	}

	bundler.reportErrors();

	Output::msgBuildSuccess();
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::routeInit()
{
	ProjectInitializer initializer(m_inputs);
	initializer.run();

	return true;
}

/*****************************************************************************/
bool Router::routeSettings(const Route inRoute)
{
	SettingsAction action = SettingsAction::Get;
	switch (inRoute)
	{
		case Route::SettingsSet:
			action = SettingsAction::Set;
			break;
		case Route::SettingsGet:
			action = SettingsAction::Get;
			break;
		case Route::SettingsUnset:
			action = SettingsAction::Unset;
			break;
		case Route::SettingsGetKeys:
			action = SettingsAction::QueryKeys;
			break;
		default:
			return false;
	}

	SettingsManager settingsMgr(m_inputs);
	if (!settingsMgr.run(action))
		return true;

	return true;
}

/*****************************************************************************/
bool Router::routeQuery()
{
	CentralState centralState(m_inputs);
	if (!centralState.initializeForList())
		return false;

	QueryController query(centralState);
	return query.printListOfRequestedType();
}

/*****************************************************************************/
bool Router::routeTerminalTest()
{
	TerminalTest termTest;
	return termTest.run();
}

/*****************************************************************************/
bool Router::parseTheme()
{
	ThemeSettingsJsonParser themeParser(m_inputs);
	if (!themeParser.serialize())
		return false;

	return true;
}

/*****************************************************************************/
bool Router::routeExport(CentralState& inCentralState)
{
	auto projectExporter = IProjectExporter::make(m_inputs.exportKind(), inCentralState);
	if (!projectExporter->generate())
		return false;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::routeDebug()
{
	LOG("Router::routeDebug()");

	Timer timer;

	std::string paths;
	Commands::forEachGlobMatch("src/*.{cpp,rc}", GlobMatch::Files, [&paths](std::string path) {
		paths += path + '\n';
	});

	paths += '\n';
	LOG(paths);
	paths.clear();
	Commands::forEachGlobMatch("src/**.{cpp,rc}", GlobMatch::Files, [&paths](std::string path) {
		paths += path + '\n';
	});

	paths += '\n';
	LOG(paths);

	LOG("glob took:", timer.asString());

	return true;
}
#endif

}
