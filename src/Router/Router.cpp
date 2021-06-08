/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Builder/AppBundler.hpp"
#include "Builder/BuildManager.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
Router::Router(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
Router::~Router()
{
	if (m_buildState != nullptr)
	{
		m_buildState->cache.saveEnvironmentCache();
		m_buildState->sourceCache.save();
	}
}

/*****************************************************************************/
bool Router::run()
{
	const auto& command = m_inputs.command();
	if (command == Route::Unknown
		|| static_cast<std::underlying_type_t<Route>>(command) >= static_cast<std::underlying_type_t<Route>>(Route::Count))
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	if (m_inputs.generator() == IdeType::Unknown)
	{
		Diagnostic::error(fmt::format("The requested IDE project generator '{}' was not recognized, or is not yet supported.", m_inputs.generatorRaw()));
		return false;
	}

	Output::lineBreak();

	if (!parseEnvFile())
		return false;

	if (command != Route::Init)
	{
		const auto& buildFile = m_inputs.buildFile();
		if (!Commands::pathExists(buildFile))
		{
			Diagnostic::error(fmt::format("Not a chalet project. '{}' was not found.", buildFile));
			return false;
		}

		m_installDependencies = true;

		m_buildState = std::make_unique<BuildState>(m_inputs);
		if (!m_buildState->initialize(m_installDependencies))
			return false;
	}

	if (!managePathVariables(m_buildState.get()))
	{
		Diagnostic::error("There was an error setting environment variables.");
		return false;
	}

	if (m_inputs.generator() != IdeType::None)
	{
		std::cout << fmt::format("generator: '{}'", m_inputs.generatorRaw()) << std::endl;
	}

	if (m_inputs.generator() == IdeType::XCode)
		return xcodebuildRoute();

	switch (command)
	{
#if defined(CHALET_DEBUG)
		case Route::Debug:
			return cmdDebug();
#endif
		case Route::Bundle: {
			chalet_assert(m_buildState != nullptr, "");
			return cmdBundle(*m_buildState);
		}

		case Route::Configure:
			return cmdConfigure();

		case Route::Init:
			return cmdInit();

		case Route::BuildRun:
		case Route::Build:
		case Route::Rebuild:
		case Route::Run:
		case Route::Clean:
		default:
			break;
	}

	chalet_assert(m_buildState != nullptr, "");
	return cmdBuild(*m_buildState);
}

/*****************************************************************************/
bool Router::cmdConfigure()
{
	// TODO: pass command to installDependencies & recheck them
	Output::msgBuildSuccess();
	return true;
}

/*****************************************************************************/
bool Router::cmdBuild(BuildState& inState)
{
	if (!inState.cache.createCacheFolder(BuildCache::Type::Local))
	{
		Diagnostic::error("There was an error creating the build cache.");
		return false;
	}

	const auto& command = m_inputs.command();

	BuildManager mgr{ m_inputs, inState };
	return mgr.run(command);
}

/*****************************************************************************/
bool Router::cmdBundle(BuildState& inState)
{
	const auto& buildFile = m_inputs.buildFile();
	if (inState.distribution.size() == 0)
	{
		Diagnostic::error(fmt::format("{}: 'bundle' object is required before creating a distribution bundle.", buildFile));
		return false;
	}

	if (!cmdBuild(inState))
		return false;

	// /*
	AppBundler bundler(inState, buildFile);

	bool result = bundler.run();
	if (result)
	{
		Output::lineBreak();
		Output::msgBuildSuccess();
		Output::lineBreak();
	}
	return result;
	// */
	// return false;
}

/*****************************************************************************/
bool Router::cmdInit()
{
	ProjectInitializer initializer{ m_inputs };
	if (!initializer.run())
		return false;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
bool Router::cmdDebug()
{
	{
		Timer timer;

		Commands::subprocess({ "echo", "Hello World!" }, false);

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

		// Commands::subprocess({ "chalet", "build", "TestingStuff" }, false);
		Commands::subprocess({ "cd", "build" }, false);
		// auto output = Commands::subprocessOutput({ "which", "chalet" }, false);
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
			Diagnostic::error(fmt::format("There was an error parsing the env file: {}", envFile));
			return false;
		}

		Diagnostic::printDone(timer.asString());
	}

	return true;
}

/*****************************************************************************/
bool Router::xcodebuildRoute()
{
#if defined(CHALET_MACOS)
	chalet_assert(m_buildState != nullptr, "");
	// Generate an XcodeGen spec in json based on the build state
	// Run xcodebuild from the command line if possible
	// This would be a lightweight BuildManager

	std::cout << "brew available: " << m_buildState->tools.brewAvailable() << "\n";

	// rm -rf build/Chalet.xcodeproj && xcodegen -s xcode-project.json -p build --use-cache

	return true;
#else
	Diagnostic::error("Xcode project generation (-g xcode) is only available on MacOS");
	return false;
#endif
}

/*****************************************************************************/
bool Router::managePathVariables(const BuildState* inState)
{
	Environment::set("CLICOLOR_FORCE", "1");

	if (inState != nullptr)
	{
		StringList outPaths = inState->environment.path();

		if (outPaths.size() > 0)
		{
			for (auto& p : outPaths)
			{
				if (!Commands::pathExists(p))
					continue;

				p = Commands::getCanonicalPath(p);
			}
			std::string appendedPath = String::join(outPaths, Path::getSeparator());

			// auto path = Environment::getPath();
			// Environment::setPath(fmt::format("{}{}{}", appendedPath, Path::getSeparator(), path));

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
