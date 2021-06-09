/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Router/Router.hpp"

#include "Builder/BuildManager.hpp"
#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Init/ProjectInitializer.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/BundleTarget.hpp"
#include "State/Target/ProjectTarget.hpp"
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

	std::unique_ptr<BuildState> buildState;

	if (command != Route::Init)
	{
		const auto& buildFile = m_inputs.buildFile();
		if (!Commands::pathExists(buildFile))
		{
			Diagnostic::error(fmt::format("Not a chalet project. '{}' was not found.", buildFile));
			return false;
		}

		m_installDependencies = true;

		buildState = std::make_unique<BuildState>(m_inputs);
		if (!buildState->initialize(m_installDependencies))
			return false;
	}

	if (!managePathVariables(buildState.get()))
	{
		Diagnostic::error("There was an error setting environment variables.");
		return false;
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
				chalet_assert(buildState != nullptr, "");
				result = cmdBundle(*buildState);
				break;
			}

			case Route::Configure:
				result = cmdConfigure();
				break;

			case Route::Init:
				result = cmdInit();
				break;

			case Route::BuildRun:
			case Route::Build:
			case Route::Rebuild:
			case Route::Run:
			case Route::Clean: {
				chalet_assert(buildState != nullptr, "");
				result = cmdBuild(*buildState);
				break;
			}

			default:
				break;
		}
	}

	if (buildState != nullptr)
		buildState->saveCaches();

	return result;
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

#if defined(CHALET_MACOS)
	bool universalBinary = false;
	for (auto& target : inState.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<BundleTarget&>(*target);
			if (bundle.macosBundle().universalBinary())
			{
				universalBinary = true;
				break;
			}
		}
	}

	if (universalBinary)
	{
		if (!buildOppositeMacosArchitecture(inState))
			return false;
	}
	else
#endif
	{
		AppBundler bundler;
		for (auto& target : inState.distribution)
		{
			if (!bundler.run(target, inState, buildFile))
				return false;
		}
	}

	Output::lineBreak();
	Output::msgBuildSuccess();
	Output::lineBreak();

	return true;
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
bool Router::buildOppositeMacosArchitecture(BuildState& inState)
{
#if defined(CHALET_MACOS)
	// TODO: Separate this into a class
	if (inState.tools.lipo().empty())
	{
		Diagnostic::error("The tool 'lipo' was not found in PATH, but is required for universal bundles.");
		return false;
	}

	auto arch = inState.info.targetArchitectureString();
	if (String::startsWith("x86_64-", arch))
		String::replaceAll(arch, "x86_64-", "arm64-");
	else
		String::replaceAll(arch, "arm64-", "x86_64-");

	CommandLineInputs inputs = m_inputs;
	inputs.setTargetArchitecture(std::move(arch));

	auto buildState = std::make_unique<BuildState>(inputs);
	if (!buildState->initialize(m_installDependencies))
		return false;

	if (!cmdBuild(*buildState))
		return false;

	auto quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	auto archUniversal = inState.info.targetArchitectureString();
	if (String::startsWith("x86_64-", archUniversal))
		String::replaceAll(archUniversal, "x86_64-", "universal-");
	else
		String::replaceAll(archUniversal, "arm64-", "universal-");

	CommandLineInputs inputsUniversal = m_inputs;
	inputsUniversal.setTargetArchitecture(std::move(archUniversal));

	auto buildStateUniversal = std::make_unique<BuildState>(inputsUniversal);
	if (!buildStateUniversal->initialize(m_installDependencies))
		return false;

	Output::setQuietNonBuild(quiet);

	auto getProjectFiles = [](BuildState& inBuildState) -> StringList {
		StringList ret;

		auto& buildOutputDir = inBuildState.paths.buildOutputDir();
		for (auto& target : inBuildState.targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<ProjectTarget&>(*target);
				if (project.isStaticLibrary())
					continue;

				ret.push_back(fmt::format("{}/{}", buildOutputDir, project.outputFile()));
			}
		}

		return ret;
	};

	StringList outputFilesA = getProjectFiles(inState);
	StringList outputFilesB = getProjectFiles(*buildState);
	StringList outputFilesUniversal = getProjectFiles(*buildStateUniversal);

	chalet_assert(outputFilesA.size() == outputFilesB.size() && outputFilesA.size() == outputFilesUniversal.size(), "");

	auto& universalBuildDir = buildStateUniversal->paths.buildOutputDir();
	if (!Commands::pathExists(universalBuildDir))
	{
		if (!Commands::makeDirectory(universalBuildDir))
		{
			Diagnostic::error(fmt::format("There was an error creating the directory: {}", universalBuildDir));
			return false;
		}
	}

	for (std::size_t i = 0; i < outputFilesA.size(); ++i)
	{
		auto& fileArchA = outputFilesA[i];
		auto& fileArchB = outputFilesB[i];
		auto& fileUniversal = outputFilesUniversal[i];

		// LOG(fileArchA, fileArchB, fileUniversal);

		if (!Commands::subprocess({ inState.tools.lipo(), "-create", "-output", fileUniversal, std::move(fileArchA), std::move(fileArchB) }))
		{
			Diagnostic::error(fmt::format("There was an error making the binary: {}", fileUniversal));
			return false;
		}
	}

	// lipo -create -output universal-apple-darwin_Release/chalet x86_64-apple-darwin_Release/chalet  arm64-apple-darwin_Release/chalet

	AppBundler bundler;
	const auto& buildFile = inputsUniversal.buildFile();
	for (auto& target : buildStateUniversal->distribution)
	{
		if (!bundler.run(target, *buildStateUniversal, buildFile))
			return false;
	}

	return true;
#else
	UNUSED(inState);
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
