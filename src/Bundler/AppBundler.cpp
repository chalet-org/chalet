/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/AppBundler.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/BundleTarget.hpp"
#include "State/Target/ScriptTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

#if defined(CHALET_WIN32)
	#include "Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Bundler/AppBundlerLinux.hpp"
#else
	#error "Unknown platform"
#endif

namespace chalet
{
namespace
{
[[nodiscard]] std::unique_ptr<IAppBundler> getAppBundler(BuildState& inState, const std::string& inBuildFile, BundleTarget& inBundle, const bool inCleanOutput)
{
#if defined(CHALET_WIN32)
	UNUSED(inBuildFile);
	return std::make_unique<AppBundlerWindows>(inState, inBundle, inCleanOutput);
#elif defined(CHALET_MACOS)
	return std::make_unique<AppBundlerMacOS>(inState, inBuildFile, inBundle, inCleanOutput);
#elif defined(CHALET_LINUX)
	UNUSED(inBuildFile);
	return std::make_unique<AppBundlerLinux>(inState, inBundle, inCleanOutput);
#else
	Diagnostic::errorAbort("Unimplemented AppBundler requested: ");
	return nullptr;
#endif
}
}

/*****************************************************************************/
bool AppBundler::run(BuildTarget& inTarget, BuildState& inState, const std::string& inBuildFile)
{
	m_cleanOutput = inState.environment.cleanOutput();

	if (inTarget->isDistributionBundle())
	{
		auto bundler = getAppBundler(inState, inBuildFile, static_cast<BundleTarget&>(*inTarget), m_cleanOutput);
		if (!removeOldFiles(*bundler))
		{
			Diagnostic::error(fmt::format("There was an error removing the previous distribution bundle for: {}", inTarget->name()));
			return false;
		}

		if (!runBundleTarget(*bundler, inState))
			return false;
	}
	else if (inTarget->isScript())
	{
		Timer buildTimer;

		if (!runScriptTarget(static_cast<const ScriptTarget&>(*inTarget), inState, inBuildFile))
			return false;

		Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
		Output::lineBreak();
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler, BuildState& inState)
{
	auto& bundle = inBundler.bundle();
	const auto& buildOutputDir = inState.paths.buildOutputDir();
	const auto& bundleProjects = inBundler.bundle().projects();

	const auto bundlePath = inBundler.getBundlePath();
	const auto executablePath = inBundler.getExecutablePath();
	const auto resourcePath = inBundler.getResourcePath();

	makeBundlePath(bundlePath, executablePath, resourcePath);

	// auto path = Environment::getPath();
	// LOG(path);

	StringList dependencies;
	StringList executables;
#if defined(CHALET_MACOS)
	StringList sharedLibraries;
#endif

	// Timer timer;

	StringList depsFromJson;
	for (auto& dep : bundle.dependencies())
	{
		if (!Commands::pathExists(dep))
			continue;

		if (!Commands::copy(dep, resourcePath, m_cleanOutput))
			return false;

		depsFromJson.push_back(dep);
	}

	for (auto& target : inState.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);

			if (!List::contains(bundleProjects, project.name()))
				continue;

			const auto& outputFile = project.outputFile();
			const auto outputFilePath = fmt::format("{}/{}", buildOutputDir, outputFile);

			if (project.isExecutable())
			{
				std::string outTarget = outputFilePath;
				List::addIfDoesNotExist(executables, std::move(outTarget));
			}
			dependencies.push_back(outputFilePath);

			if (bundle.includeDependentSharedLibraries() && !project.isStaticLibrary())
			{
				if (!inState.tools.getExecutableDependencies(outputFilePath, dependencies))
				{
					Diagnostic::error(fmt::format("getExecutableDependencies error for file '{}'.", outputFilePath));
					return false;
				}
			}
		}
	}
	bundle.addDependencies(dependencies);

	if (bundle.includeDependentSharedLibraries())
	{
		StringList projectNames;
		for (auto& target : inState.targets)
		{
			if (target->isProject())
			{
				auto& project = static_cast<const ProjectTarget&>(*target);
				projectNames.push_back(String::getPathFilename(project.outputFile()));
			}
		}

		StringList depsOfDeps;
		for (auto& dep : dependencies)
		{
			if (dep.empty() || List::contains(depsFromJson, dep) || List::contains(projectNames, dep))
				continue;

			auto depPath = Commands::which(dep);
			if (depPath.empty())
			{
				Diagnostic::warn(fmt::format("Dependency not found in path: '{}'", dep));
				continue;
			}

			if (!inState.tools.getExecutableDependencies(depPath, depsOfDeps))
			{
				Diagnostic::error(fmt::format("Dependencies not found for file: '{}'", depPath));
				return false;
			}
		}

		bundle.addDependencies(depsOfDeps);
	}

	bundle.sortDependencies();

	// LOG("Distribution dependencies gathered in:", timer.asString());

	uint copyCount = 0;
	for (auto& dep : bundle.dependencies())
	{
		if (List::contains(depsFromJson, dep))
			continue;

		if (!Commands::pathExists(dep))
			continue;

		if (!Commands::copy(dep, executablePath, m_cleanOutput))
			return false;

		++copyCount;

#if !defined(CHALET_WIN32)
		if (List::contains(executables, dep))
		{
			const auto filename = String::getPathFilename(dep);
			const auto executable = fmt::format("{}/{}", executablePath, filename);

			if (!Commands::setExecutableFlag(executable, m_cleanOutput))
				return false;
		}
#endif
	}

	Commands::forEachFileMatch(resourcePath, bundle.excludes(), [this](const fs::path& inPath) {
		Commands::remove(inPath.string(), m_cleanOutput);
	});

	if (copyCount > 0)
	{
		// Output::lineBreak();
	}

	if (!inBundler.bundleForPlatform())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptTarget& inScript, BuildState& inState, const std::string& inBuildFile)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	if (!inScript.description().empty())
		Output::msgScriptDescription(inScript.description());
	else
		Output::msgScript(inScript.name());

	Output::lineBreak();

	ScriptRunner scriptRunner(inState.tools, inBuildFile, m_cleanOutput);
	if (!scriptRunner.run(scripts))
	{
		Output::lineBreak();
		Output::msgBuildFail(); // TODO: Script failed
		Output::lineBreak();
		return false;
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::removeOldFiles(IAppBundler& inBundler)
{
	auto& bundle = inBundler.bundle();
	const auto& outDir = bundle.outDir();

	if (!List::contains(m_removedDirs, outDir))
	{
		Commands::removeRecursively(outDir, m_cleanOutput);
		m_removedDirs.push_back(outDir);
	}

	if (!inBundler.removeOldFiles())
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath)
{
	StringList dirList{ inBundlePath };
	List::addIfDoesNotExist(dirList, inExecutablePath);
	List::addIfDoesNotExist(dirList, inResourcePath);

	// make prod dir
	for (auto& dir : dirList)
	{
		if (Commands::pathExists(dir))
			continue;

		if (!Commands::makeDirectory(dir, m_cleanOutput))
			return false;
	}

	return true;
}
}
