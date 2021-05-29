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

		m_buildState = std::make_unique<BuildState>(m_inputs);

		if (!parseCacheJson())
			return false;

		if (!parseBuildJson(buildFile))
			return false;

		fetchToolVersions();

		if (!m_buildState->initializeBuild())
			return false;
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

	if (m_inputs.generator() == IdeType::XCode)
		return xcodebuildRoute();

	return m_routes[command](*this);
}

/*****************************************************************************/
bool Router::cmdConfigure()
{
	if (!installDependencies())
		return false;

	// TODO: pass command to installDependencies & recheck them
	Output::msgBuildSuccess();
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

	const auto& buildFile = m_inputs.buildFile();
	if (!m_buildState->bundle.exists())
	{
		Diagnostic::error(fmt::format("{}: 'bundle' object is required before creating a distribution bundle.", buildFile));
		return false;
	}

	if (!cmdBuild())
		return false;

	AppBundler bundler(*m_buildState, buildFile);

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

		enforceArchitectureInPath();

		Diagnostic::printDone(timer.asString());
	}

	return true;
}

/*****************************************************************************/
bool Router::parseCacheJson()
{
	chalet_assert(m_buildState != nullptr, "");

	CacheJsonParser parser(m_inputs, *m_buildState);
	return parser.serialize();
}

/*****************************************************************************/
bool Router::parseBuildJson(const std::string& inFile)
{
	chalet_assert(m_buildState != nullptr, "");

	BuildJsonParser parser(m_inputs, *m_buildState, inFile);
	return parser.serialize();
}

/*****************************************************************************/
void Router::fetchToolVersions()
{
	Timer timer;

	Diagnostic::info("Verifying Ancillary Tools", false);

	m_buildState->tools.fetchVersions();

	Diagnostic::printDone(timer.asString());
}

/*****************************************************************************/
bool Router::installDependencies()
{
	chalet_assert(m_buildState != nullptr, "");

	const auto& command = m_inputs.command();
	const bool cleanOutput = true;

	DependencyManager depMgr(*m_buildState, cleanOutput);
	if (!depMgr.run(command == Route::Configure))
	{
		Diagnostic::error("There was an error creating the dependencies.");
		return false;
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
bool Router::managePathVariables()
{
	Environment::set("CLICOLOR_FORCE", "1");

	if (m_buildState != nullptr)
	{
#if !defined(CHALET_WIN32)
		// This is needed on linux to look for additional libraries at runtime
		// TODO: This might actually vary between distros. Figure out if any of this is needed?

		// This is needed so ldd can resolve the correct file dependencies
		StringList outPaths = m_buildState->environment.path();

		for (auto& p : outPaths)
		{
			if (!Commands::pathExists(p))
				continue;

			p = Commands::getCanonicalPath(p);
		}

		// LD_LIBRARY_PATH - dynamic link paths
		// LIBRARY_PATH - static link paths
		// For now, just use the same paths for both
		const auto kLdLibraryPath = "LD_LIBRARY_PATH";
		const auto kLibraryPath = "LIBRARY_PATH";

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

/*****************************************************************************/
void Router::enforceArchitectureInPath()
{
#if defined(CHALET_WIN32)
	auto path = Environment::getPath();
	if (String::contains({ "/mingw64/", "/mingw32/" }, path))
	{

		if (m_buildState)
		{
			std::string lower = String::toLowerCase(path);
			auto targetArch = m_buildState->info.targetArchitecture();
			if (targetArch == Arch::Cpu::X64)
			{
				auto start = lower.find("/mingw32/");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "/mingw64/");
					Environment::setPath(path);
				}
			}
			else if (targetArch == Arch::Cpu::X86)
			{
				auto start = lower.find("/mingw64/");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "/mingw32/");
					Environment::setPath(path);
				}
			}
		}
	}
	else if (String::contains({ "/clang64/", "/clang32/" }, path))
	{
		// TODO: clangarm64

		if (m_buildState)
		{
			std::string lower = String::toLowerCase(path);
			auto targetArch = m_buildState->info.targetArchitecture();
			if (targetArch == Arch::Cpu::X64)
			{
				auto start = lower.find("/clang32/");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "/clang64/");
					Environment::setPath(path);
				}
			}
			else if (targetArch == Arch::Cpu::X86)
			{
				auto start = lower.find("/clang64/");
				if (start != std::string::npos)
				{
					String::replaceAll(path, path.substr(start, 9), "/clang32/");
					Environment::setPath(path);
				}
			}
		}
	}
#else
#endif
}

}
