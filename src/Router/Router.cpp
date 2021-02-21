/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "BuildJson/BuildJsonParser.hpp"
#include "Builder/AppBundler.hpp"
#include "Builder/BuildManager.hpp"
#include "CacheJson/CacheJsonParser.hpp"
#include "Dependencies/DependencyManager.hpp"
#include "Init/ProjectInitializer.hpp"
#include "Libraries/Format.hpp"
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
	m_inputs(inInputs),
	m_routes({
		{ Route::BuildRun, &Router::cmdBuild },
		{ Route::Build, &Router::cmdBuild },
		{ Route::Rebuild, &Router::cmdBuild },
		{ Route::Run, &Router::cmdBuild },
		{ Route::Bundle, &Router::cmdBundle },
		{ Route::Clean, &Router::cmdBuild },
		// { Routes::Profile, &Router::cmdBuild },
		{ Route::Install, &Router::cmdInstall },
		{ Route::Configure, &Router::cmdConfigure },
		{ Route::Init, &Router::cmdInit },
	})
{
#if defined(CHALET_DEBUG)
	m_routes.emplace(Route::Debug, &Router::cmdDebug);
#endif
}

/*****************************************************************************/
Router::~Router()
{
	if (m_buildState != nullptr)
		m_buildState->cache.saveEnvironmentCache();
}

/*****************************************************************************/
bool Router::run()
{
	const auto& command = m_inputs.command();
	if (m_routes.find(command) == m_routes.end())
	{
		Diagnostic::error("Command not recognized.");
		return false;
	}

	if (m_inputs.generator() == IdeType::Unknown)
	{
		Diagnostic::error(fmt::format("The requested IDE project generator '{}' was not recognized, or is not yet supported.", m_inputs.generatorRaw()));
		return false;
	}

	if (command != Route::Init)
	{
		const auto& file = CommandLineInputs::file();
		if (!Commands::pathExists(file))
		{
			Diagnostic::error(fmt::format("Not a chalet project. '{}' was not found.", file));
			return false;
		}

		Output::lineBreak();

		m_buildState = std::make_unique<BuildState>(m_inputs);

		if (!parseCacheJson())
			return false;

		if (!parseBuildJson(file))
			return false;

		m_buildState->initializeBuild(m_inputs);
	}

	if (!managePathVariables())
	{
		Diagnostic::error("There was an error setting environment variables.");
		return false;
	}

	if (m_inputs.generator() != IdeType::None)
	{
		std::cout << fmt::format("generator: '{}'", m_inputs.generatorRaw()) << std::endl;
	}

	return m_routes[command](*this);
}

/*****************************************************************************/
bool Router::cmdInstall()
{
	if (!installDependencies())
		return false;

	// TODO: pass command to installDependencies & recheck them
	Output::msgBuildSuccess();
	return true;
}

/*****************************************************************************/
bool Router::cmdConfigure()
{
	// TODO: Compare cache.json & build.json and make any adjustments to cache,
	//   but ultimately, we don't want to fetch external dependencies, nor build of course
	return true;
}

/*****************************************************************************/
bool Router::cmdBuild()
{
	chalet_assert(m_buildState != nullptr, "");

	if (!installDependencies())
		return false;

	if (!m_buildState->cache.createCacheFolder(BuildCache::Type::Local))
	{
		Diagnostic::error("There was an error creating the build cache.");
		return false;
	}

	const auto& command = m_inputs.command();

	BuildManager mgr{ m_inputs, *m_buildState };
	return mgr.run(command);
}

/*****************************************************************************/
bool Router::cmdBundle()
{
	chalet_assert(m_buildState != nullptr, "");

	if (!m_buildState->bundle.exists())
	{
		const auto& file = CommandLineInputs::file();
		Diagnostic::error(fmt::format("{}: 'bundle' object is required before creating a distribution bundle.", file));
		return false;
	}

	if (!cmdBuild())
		return false;

	AppBundler bundler(*m_buildState);

	bool result = bundler.run();
	if (result)
	{
		Output::msgBuildSuccess();
		Output::lineBreak();
	}
	return result;
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
bool Router::parseCacheJson()
{
	chalet_assert(m_buildState != nullptr, "");

	Timer timer;

	{
		CacheJsonParser parser(m_inputs, *m_buildState);
		if (!parser.serialize())
			return false;
	}

	auto result = timer.stop();
	const auto& file = m_buildState->cache.environmentCache().filename();
	Output::print(Color::Reset, fmt::format("{} parsed in: {}ms", file, result));

	return true;
}

/*****************************************************************************/
bool Router::parseBuildJson(const std::string& inFile)
{
	chalet_assert(m_buildState != nullptr, "");

	Timer timer;

	{
		BuildJsonParser parser(m_inputs, *m_buildState, inFile);
		if (!parser.serialize())
			return false;
	}

	auto result = timer.stop();
	Output::print(Color::Reset, fmt::format("{} parsed in: {}ms\n", inFile, result));

	return true;
}

/*****************************************************************************/
bool Router::installDependencies()
{
	chalet_assert(m_buildState != nullptr, "");

	const auto& command = m_inputs.command();
	const bool cleanOutput = true;

	DependencyManager depMgr(*m_buildState, cleanOutput);
	if (!depMgr.run(command == Route::Install))
	{
		Diagnostic::error("There was an error creating the dependencies.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool Router::managePathVariables()
{
	Environment::set("CLICOLOR_FORCE", "1");

	if (m_buildState != nullptr)
	{
#if defined(CHALET_LINUX)
		// This is needed on linux to look for additional libraries at runtime
		// TODO: This might actually vary between distros. Figure out if any of this is needed?

		// This is needed so ldd can resolve the correct file dependencies
		StringList outPaths = m_buildState->environment.path();

		for (auto& p : outPaths)
		{
			if (!Commands::pathExists(p))
			{
				std::cout << "Not found: " << p << std::endl;
				continue;
			}
			p = Commands::getCanonicalPath(p);
		}

		// LD_LIBRARY_PATH - dynamic link paths
		// LIBRARY_PATH - static link paths
		// For now, just use the same paths for both
		constexpr std::string_view kLdLibraryPath = "LD_LIBRARY_PATH";
		constexpr std::string_view kLibraryPath = "LIBRARY_PATH";

		{
			std::string libraryPath = String::join(outPaths, ':');
			std::string ldLibraryPath = libraryPath;

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
#endif
	}
	return true;
}

}
