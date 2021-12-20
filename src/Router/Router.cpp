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
#include "Process/ProcessController.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/ColorTest.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

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
	if (route == Route::Query)
		return routeQuery();
	else if (route == Route::ColorTest)
		return routeColorTest();

	if (route == Route::Unknown
		|| static_cast<std::underlying_type_t<Route>>(route) >= static_cast<std::underlying_type_t<Route>>(Route::Count))
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	if (m_inputs.generator() == IdeType::Unknown)
	{
		Diagnostic::error("The requested IDE project generator '{}' was not recognized, or is not yet supported.", m_inputs.generatorRaw());
		return false;
	}

	Unique<StatePrototype> prototype;
	Unique<BuildState> buildState;

	const bool isSettings = route == Route::SettingsGet
		|| route == Route::SettingsSet
		|| route == Route::SettingsUnset
		|| route == Route::SettingsGetKeys;

	if (route != Route::Init && !isSettings)
	{
		Output::lineBreak();

		prototype = std::make_unique<StatePrototype>(m_inputs);

		if (!prototype->initialize())
			return false;

		if (route != Route::Bundle)
		{
			chalet_assert(prototype != nullptr, "");
			auto inputs = m_inputs;
			buildState = std::make_unique<BuildState>(std::move(inputs), *prototype);
			if (!buildState->initialize())
				return false;
		}
	}

	if (m_inputs.generator() != IdeType::None)
	{
		LOG(fmt::format("generator: '{}'", m_inputs.generatorRaw()));
	}

	bool result = false;

	if (m_inputs.generator() == IdeType::XCode)
	{
		chalet_assert(buildState != nullptr, "");
		result = routeXcodeGenTest(*buildState);
	}
	else
	{
		switch (route)
		{
#if defined(CHALET_DEBUG)
			case Route::Debug: {
				result = routeDebug();
				break;
			}
#endif
			case Route::Bundle: {
				chalet_assert(prototype != nullptr, "");
				result = routeBundle(*prototype);
				break;
			}

			case Route::Configure: {
				chalet_assert(buildState != nullptr, "");
				result = routeConfigure(*buildState);
				break;
			}

			case Route::Init: {
				result = routeInit();
				break;
			}

			case Route::SettingsGet:
			case Route::SettingsSet:
			case Route::SettingsUnset:
			case Route::SettingsGetKeys: {
				result = routeSettings(route);
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

			default:
				break;
		}
	}

	if (prototype != nullptr)
		prototype->saveCaches();

	return result;
}

/*****************************************************************************/
bool Router::routeConfigure(BuildState& inState)
{
	// TODO: pass route to installDependencies & recheck them

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
bool Router::routeBundle(StatePrototype& inPrototype)
{
	const auto& inputFile = m_inputs.inputFile();
	if (inPrototype.distribution.empty())
	{
		Diagnostic::error("{}: There are no distribution targets: missing 'distribution'", inputFile);
		return false;
	}

	/*if (inPrototype.requiredBuildConfigurations().empty())
	{
		Diagnostic::error("{}: 'bundle' ran without any valid distribution bundles: missing 'configuration'", inputFile);
		return false;
	}*/

	AppBundler bundler(m_inputs, inPrototype);

	if (!bundler.runBuilds())
		return false;

	for (auto& target : inPrototype.distribution)
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
	StatePrototype prototype(m_inputs);
	if (!prototype.initializeForList())
		return false;

	QueryController query(m_inputs, prototype);
	return query.printListOfRequestedType();
}

/*****************************************************************************/
bool Router::routeColorTest()
{
	ColorTest colorTest;
	return colorTest.run();
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
bool Router::routeXcodeGenTest(BuildState& inState)
{
#if defined(CHALET_MACOS)
	// Generate an XcodeGen spec in json based on the build state
	// Run xcodebuild from the command line if possible
	// This would be a lightweight BuildManager

	LOG("brew available:", inState.tools.brewAvailable());

	// rm -rf build/Chalet.xcodeproj && xcodegen -s xcode-project.json -p build --use-cache

	return true;
#else
	UNUSED(inState);
	Diagnostic::error("Xcode project generation (-g xcode) is only available on MacOS");
	return false;
#endif
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::routeDebug()
{
	LOG("Router::routeDebug()");

	return true;
}
#endif

}
