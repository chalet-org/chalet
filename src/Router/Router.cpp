/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"

#include "Settings/SettingsAction.hpp"
#include "Settings/SettingsManager.hpp"
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

	const auto& buildFile = m_inputs.buildFile();
	const bool isSettings = command == Route::SettingsGet || command == Route::SettingsSet || command == Route::SettingsUnset;
	if (command != Route::Init && !isSettings)
	{
		Output::lineBreak();

		if (!parseEnvFile())
			return false;

		if (!Commands::pathExists(buildFile))
		{
			Diagnostic::error("Not a chalet project. '{}' was not found.", buildFile);
			return false;
		}

		prototype = std::make_unique<StatePrototype>(m_inputs, buildFile);

		if (!prototype->initialize())
			return false;

		if (command != Route::Bundle && command != Route::Configure)
		{
			chalet_assert(prototype != nullptr, "");
			buildState = std::make_unique<BuildState>(m_inputs, *prototype);
			if (!buildState->initialize())
				return false;
		}
	}

	if (!isSettings)
	{
		if (!managePathVariables(prototype.get()))
		{
			Diagnostic::error("There was an error setting environment variables.");
			return false;
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
	Output::lineBreak();
	Output::msgConfigureCompleted();
	Output::lineBreak();

	return true;
}

/*****************************************************************************/
bool Router::cmdBundle(StatePrototype& inPrototype)
{
	const auto& buildFile = m_inputs.buildFile();
	if (inPrototype.requiredBuildConfigurations().size() == 0)
	{
		Diagnostic::error("{}: 'bundle' ran without any valid distribution bundles: missing 'configuration'", buildFile);
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
	{
		Timer timer;

		Commands::subprocess({ "echo", "Hello World!" });

		auto result = timer.stop();
		Output::print(Color::Reset, fmt::format("time: {}ms\n", result));
	}

	{
		Timer timer;

		// StringList patterns{ "*.cpp" };
		// Commands::forEachFileMatch("src", patterns, [](const fs::path& inPath) {
		// 	std::cout << inPath.string() << '\n';
		// });

		// std::cout << std::endl;

		// Commands::subprocess({ "chalet", "build", "TestingStuff" });
		Commands::subprocess({ "cd", "build" });
		// auto output = Commands::subprocessOutput({ "which", "chalet" });
		// LOG(output);

		auto result = timer.stop();
		Output::print(Color::Reset, fmt::format("time: {}ms\n", result));
	}

	return true;
}
#endif

/*****************************************************************************/
bool Router::parseEnvFile()
{
	std::string envFile = m_inputs.envFile();
	if (String::equals(m_inputs.defaultEnvFile(), envFile))
	{
#if defined(CHALET_WIN32)
		std::string toSearch{ ".env.windows" };
#elif defined(CHALET_MACOS)
		std::string toSearch{ ".env.macos" };
#else
		std::string toSearch{ ".env.linux" };
#endif
		if (Commands::pathExists(toSearch))
			envFile = std::move(toSearch);
	}

	if (Commands::pathExists(envFile))
	{
		Timer timer;
		Diagnostic::info(fmt::format("Reading Environment [{}]", envFile), false);

		if (!Environment::parseVariablesFromFile(envFile))
		{
			Diagnostic::error("There was an error parsing the env file: {}", envFile);
			return false;
		}

		Diagnostic::printDone(timer.asString());
	}

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

/*****************************************************************************/
bool Router::managePathVariables(const StatePrototype* inPrototype)
{
	Environment::set("CLICOLOR_FORCE", "1");

	if (inPrototype != nullptr)
	{
		StringList outPaths = inPrototype->environment.path();

		if (outPaths.size() > 0)
		{
			for (auto& p : outPaths)
			{
				if (!Commands::pathExists(p))
					continue;

				p = Commands::getCanonicalPath(p);
			}
			std::string appendedPath = String::join(outPaths, Path::getSeparator());

			// Note: Not needed on mac: @rpath stuff is done instead
#if defined(CHALET_LINUX)
			// This is needed on linux to look for additional libraries at runtime
			// TODO: This might actually vary between distros. Figure out if any of this is needed?

			// This is needed so ldd can resolve the correct file dependencies

			// LD_LIBRARY_PATH - dynamic link paths
			// LIBRARY_PATH - static link paths
			// For now, just use the same paths for both
			const auto kLdLibraryPath = "LD_LIBRARY_PATH";
			const auto kLibraryPath = "LIBRARY_PATH";

			{
				std::string libraryPath = appendedPath;
				std::string ldLibraryPath = appendedPath;

				auto oldLd = Environment::get(kLdLibraryPath);
				if (oldLd != nullptr)
					ldLibraryPath = ldLibraryPath.empty() ? oldLd : fmt::format("{}:{}", ldLibraryPath, oldLd);

				auto old = Environment::get(kLibraryPath);
				if (old != nullptr)
					libraryPath = libraryPath.empty() ? oldLd : fmt::format("{}:{}", libraryPath, old);

				// LOG(ldLibraryPath);

				Environment::set(kLdLibraryPath, ldLibraryPath);
				Environment::set(kLibraryPath, libraryPath);
			}
#else
			UNUSED(appendedPath);
#endif
		}
	}
	return true;
}

}
