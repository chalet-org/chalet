/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Router/Router.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Builder/BatchValidator.hpp"
#include "Export/IProjectExporter.hpp"
#include "Process/Environment.hpp"
#include "Process/ProcessController.hpp"
#include "Query/QueryController.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "System/UpdateNotifier.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Terminal/TerminalTest.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonValues.hpp"

#include "Libraries/Json.hpp"

#include <chrono>
#include <thread>

namespace chalet
{
namespace
{
using RouteAction = std::function<bool(Router&)>;
using RouteList = std::unordered_map<RouteType, RouteAction>;
}
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

	if (workingDirectoryIsGlobalChaletDirectory())
		return false;

	const auto& route = m_inputs.route();
	if (route.isUnknown())
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	// Routes that don't require state
	switch (route.type())
	{
		case RouteType::Query:
			return routeQuery();

		case RouteType::TerminalTest:
			return routeTerminalTest();

		case RouteType::Init:
			return routeInit();

		case RouteType::SettingsGet:
		case RouteType::SettingsSet:
		case RouteType::SettingsUnset:
		case RouteType::SettingsGetKeys:
			return routeSettings();

		case RouteType::Validate:
			return routeValidate();

#if defined(CHALET_DEBUG)
		case RouteType::Debug:
			return routeDebug();
#endif

		default: break;
	}

	return runRoutesThatRequireState();
}

/*****************************************************************************/
bool Router::runRoutesThatRequireState()
{
	const auto& route = m_inputs.route();
	if (route.isExport() && m_inputs.exportKind() == ExportKind::None)
	{
		Diagnostic::error("The requested project kind '{}' was not recognized, or is not yet supported.", m_inputs.exportKindRaw());
		return false;
	}

	auto centralState = std::make_unique<CentralState>(m_inputs);
	Unique<BuildState> buildState;

	if (!centralState->initialize())
		return false;

	if (!route.isExport())
	{
		buildState = std::make_unique<BuildState>(centralState->inputs(), *centralState);
		if (!buildState->initialize())
			return false;

		// Local settings needs to be available for sub-chalet targets
		centralState->cache.saveSettings(SettingsType::Local);
	}

	bool result = false;
	switch (route.type())
	{
		case RouteType::Bundle: {
			chalet_assert(buildState != nullptr, "");
			result = routeBundle(*buildState);
			break;
		}

		case RouteType::Configure: {
			chalet_assert(buildState != nullptr, "");
			result = routeConfigure(*buildState);
			break;
		}

		case RouteType::BuildRun:
		case RouteType::Build:
		case RouteType::Rebuild:
		case RouteType::Run:
		case RouteType::Clean: {
			chalet_assert(buildState != nullptr, "");
			result = buildState->doBuild(m_inputs.route());
			break;
		}

		case RouteType::Export: {
			result = routeExport(*centralState);
			break;
		}

		default:
			break;
	}

	if (centralState != nullptr)
	{
		UpdateNotifier updater(*centralState);
		updater.notifyForUpdates();

		centralState->saveCaches();
	}

	return result;
}

/*****************************************************************************/
bool Router::routeConfigure(BuildState& inState)
{
	if (!inState.generateProjects())
		return false;

	Output::lineBreak();
	Output::msgConfigureCompleted(inState.workspace.metadata().name());
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::routeBundle(BuildState& inState)
{
	const auto& inputFile = inState.inputs.inputFile();
	if (inState.distribution.empty())
	{
		Diagnostic::error("{}: There are no distribution targets: missing 'distribution'", inputFile);
		return false;
	}

	// We always want to build all targets during the bundle step
	inState.inputs.setLastTarget(Values::All);

	AppBundler bundler(inState);
	{
		CommandRoute route(RouteType::Build);
		if (!inState.doBuild(route, false))
			return false;
	}

	for (auto& target : inState.distribution)
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
bool Router::routeSettings()
{
	const auto& route = m_inputs.route();

	SettingsManager settingsMgr(m_inputs);
	if (!settingsMgr.run(static_cast<SettingsAction>(route.type())))
		return true;

	return true;
}

/*****************************************************************************/
bool Router::routeValidate()
{
	auto& schema = m_inputs.settingsFile();
	StringList files;
	auto& argumentsOpt = m_inputs.runArguments();
	if (argumentsOpt.has_value())
	{
		auto& arguments = *argumentsOpt;
		for (const auto& val : arguments)
		{
			if (!Commands::addPathToListWithGlob(std::string(val), files, GlobMatch::FilesAndFolders))
				return false;
		}
	}

	if (schema.empty() || !Commands::pathExists(schema))
	{
		Diagnostic::error("Schema file for the validation doesn't exist: {}", schema);
		return false;
	}

	for (auto& file : files)
	{
		if (file.empty() || !Commands::pathExists(file))
		{
			Diagnostic::error("File for the validation doesn't exist: {}", file);
			return false;
		}
	}

	Diagnostic::info("Validating files against the selected schema");

	Output::lineBreak();

	BatchValidator validator(nullptr, schema);
	bool result = validator.validate(files, false);

	return result;
}

/*****************************************************************************/
bool Router::routeQuery()
{
	CentralState centralState(m_inputs);
	if (!centralState.initializeForQuery())
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
bool Router::routeExport(CentralState& inCentralState)
{
	// We want export to assume all targets are needed
	m_inputs.setLastTarget(Values::All);

	auto projectExporter = IProjectExporter::make(m_inputs.exportKind(), m_inputs);
	if (!projectExporter->generate(inCentralState))
		return false;

	Output::lineBreak();
	Output::msgBuildSuccess();
	Output::lineBreak();

	return true;
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
bool Router::workingDirectoryIsGlobalChaletDirectory()
{
	const auto& cwd = m_inputs.workingDirectory();
	const auto globalDirectory = m_inputs.getGlobalDirectory();

	if (String::startsWith(globalDirectory, cwd))
	{
		auto folder = String::getPathFilename(globalDirectory);
		Diagnostic::error("Cannot run commands from the '{}' path - not allowed.", folder);
		return true;
	}

	return false;
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::routeDebug()
{
	LOG("Router::routeDebug()");

	Diagnostic::infoEllipsis("Testing");

	Commands::sleep(5);
	Diagnostic::printDone();

	return true;
}
#endif

}
