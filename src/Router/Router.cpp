/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"

#include "Process/Process.hpp"
#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/ThemeSettingsJsonParser.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/BundleTarget.hpp"
#include "State/StatePrototype.hpp"
#include "State/Target/ProjectTarget.hpp"
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

	Route command = m_inputs.command();
	if (command == Route::Unknown
		|| static_cast<std::underlying_type_t<Route>>(command) >= static_cast<std::underlying_type_t<Route>>(Route::Count))
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	if (m_inputs.generator() == IdeType::Unknown)
	{
		Diagnostic::error("The requested IDE project generator '{}' was not recognized, or is not yet supported.", m_inputs.generatorRaw());
		return false;
	}

	std::unique_ptr<StatePrototype> prototype;
	std::unique_ptr<BuildState> buildState;

	const bool isSettings = command == Route::SettingsGet || command == Route::SettingsSet || command == Route::SettingsUnset;
	if (command != Route::Init && !isSettings)
	{
		Output::lineBreak();

		if (!parseEnvFile())
			return false;

		prototype = std::make_unique<StatePrototype>(m_inputs);

		if (!prototype->initialize())
		{
			Output::lineBreak();
			return false;
		}

		if (command != Route::Bundle && command != Route::Configure)
		{
			chalet_assert(prototype != nullptr, "");
			buildState = std::make_unique<BuildState>(m_inputs, *prototype);
			if (!buildState->initialize())
			{
				Output::lineBreak();
				return false;
			}
		}
	}

	if (m_inputs.generator() != IdeType::None)
	{
		std::cout << fmt::format("generator: '{}'", m_inputs.generatorRaw()) << std::endl;
	}

	bool result = false;

	if (m_inputs.generator() == IdeType::XCode)
	{
		chalet_assert(buildState != nullptr, "");
		result = xcodebuildRoute(*buildState);
	}
	else
	{
		switch (command)
		{
#if defined(CHALET_DEBUG)
			case Route::Debug:
				result = cmdDebug();
				break;
#endif
			case Route::Bundle: {
				chalet_assert(prototype != nullptr, "");
				result = cmdBundle(*prototype);
				break;
			}

			case Route::Configure:
				result = cmdConfigure();
				break;

			case Route::Init:
				result = cmdInit();
				break;

			case Route::SettingsGet:
			case Route::SettingsSet:
			case Route::SettingsUnset:
				result = cmdSettings(command);
				break;

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
bool Router::cmdConfigure()
{
	// TODO: pass command to installDependencies & recheck them
	// Output::lineBreak();
	Output::msgConfigureCompleted();
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::cmdBundle(StatePrototype& inPrototype)
{
	const auto& inputFile = m_inputs.inputFile();
	if (inPrototype.requiredBuildConfigurations().size() == 0)
	{
		Diagnostic::error("{}: 'bundle' ran without any valid distribution bundles: missing 'configuration'", inputFile);
		return false;
	}

	AppBundler bundler(m_inputs, inPrototype);

	if (!bundler.runBuilds())
		return false;

	for (auto& target : inPrototype.distribution)
	{
		if (!bundler.run(target))
			return false;
	}

	Output::msgBuildSuccess();
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::cmdInit()
{
	ProjectInitializer initializer(m_inputs);
	if (!initializer.run())
		return true;

	return true;
}

/*****************************************************************************/
bool Router::cmdSettings(const Route inRoute)
{
	if (m_inputs.settingsType() == SettingsType::None)
	{
		Diagnostic::error("There was an error determining the settings request");
		return false;
	}

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
		default:
			return false;
	}

	SettingsManager settingsMgr(m_inputs, action);
	if (!settingsMgr.run())
		return true;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::cmdDebug()
{
	LOG("Router::cmdDebug()");

	return true;
}
#endif

/*****************************************************************************/
bool Router::parseTheme()
{
	ThemeSettingsJsonParser themeParser(m_inputs);
	if (!themeParser.serialize())
		return false;

	return true;
}

/*****************************************************************************/
bool Router::parseEnvFile()
{
	auto searchDotEnv = [this](const std::string& inRelativeEnv, const std::string& inEnv) -> std::string {
		if (String::endsWith(m_inputs.defaultEnvFile(), inRelativeEnv))
		{
			auto toSearch = String::getPathFolder(inRelativeEnv);
			if (!toSearch.empty())
			{
				toSearch = fmt::format("{}/{}", toSearch, inEnv);
				if (Commands::pathExists(toSearch))
					return toSearch;
			}

			if (Commands::pathExists(inEnv))
				return inEnv;
		}
		else
		{
			if (Commands::pathExists(inRelativeEnv))
				return inRelativeEnv;

			if (Commands::pathExists(inEnv))
				return inEnv;
		}

		return std::string();
	};

#if defined(CHALET_WIN32)
	std::string platformEnv = fmt::format("{}.windows", m_inputs.defaultEnvFile());
#elif defined(CHALET_MACOS)
	std::string platformEnv = fmt::format("{}.macos", m_inputs.defaultEnvFile());
#else
	std::string platformEnv = fmt::format("{}.linux", m_inputs.defaultEnvFile());
#endif

	std::string envFile = searchDotEnv(m_inputs.envFile(), platformEnv);
	if (envFile.empty())
	{
		envFile = searchDotEnv(m_inputs.envFile(), m_inputs.defaultEnvFile());
	}

	if (envFile.empty())
	{
		if (Commands::pathExists(m_inputs.envFile()))
			envFile = m_inputs.envFile();
		else
			return true; // don't care
	}

	Timer timer;
	Diagnostic::infoEllipsis("Reading Environment [{}]", envFile);

	if (!Environment::parseVariablesFromFile(envFile))
	{
		Diagnostic::error("There was an error parsing the env file: {}", envFile);
		return false;
	}

	Diagnostic::printDone(timer.asString());
	return true;
}

/*****************************************************************************/
bool Router::xcodebuildRoute(BuildState& inState)
{
#if defined(CHALET_MACOS)
	// Generate an XcodeGen spec in json based on the build state
	// Run xcodebuild from the command line if possible
	// This would be a lightweight BuildManager

	std::cout << "brew available: " << inState.tools.brewAvailable() << "\n";

	// rm -rf build/Chalet.xcodeproj && xcodegen -s xcode-project.json -p build --use-cache

	return true;
#else
	UNUSED(inState);
	Diagnostic::error("Xcode project generation (-g xcode) is only available on MacOS");
	return false;
#endif
}

}
