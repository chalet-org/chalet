/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/AppBundler.hpp"

#include "Builder/BuildManager/ScriptRunner.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

#if defined(CHALET_WIN32)
	#include "Builder/Bundler/AppBundlerWindows.hpp"
#elif defined(CHALET_MACOS)
	#include "Builder/Bundler/AppBundlerMacOS.hpp"
#elif defined(CHALET_LINUX)
	#include "Builder/Bundler/AppBundlerLinux.hpp"
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
AppBundler::AppBundler(BuildState& inState, const std::string& inBuildFile) :
	m_state(inState),
	m_buildFile(inBuildFile)
{

	m_cleanOutput = m_state.environment.cleanOutput();
}

/*****************************************************************************/
bool AppBundler::run()
{
	for (auto& target : m_state.distribution)
	{

		if (target->isDistributionBundle())
		{
			auto bundler = getAppBundler(m_state, m_buildFile, static_cast<BundleTarget&>(*target), m_cleanOutput);
			if (!removeOldFiles(*bundler))
			{
				Diagnostic::error(fmt::format("There was an error removing the previous distribution bundle for: {}", target->name()));
				return false;
			}

			if (!runBundleTarget(*bundler))
				return false;
		}
		else if (target->isScript())
		{
			Timer buildTimer;

			if (!runScriptTarget(static_cast<const ScriptTarget&>(*target)))
				return false;

			Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
			Output::lineBreak();
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::runBundleTarget(IAppBundler& inBundler)
{
	auto& bundle = inBundler.bundle();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
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

	StringList depsFromJson;
	for (auto& dep : bundle.dependencies())
	{
		if (!Commands::pathExists(dep))
			continue;

		if (!Commands::copy(dep, resourcePath, m_cleanOutput))
			return false;

		depsFromJson.push_back(dep);
	}

	for (auto& target : m_state.targets)
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

			if (bundle.includeDependentSharedLibraries())
			{
				if (!m_state.tools.getExecutableDependencies(outputFilePath, dependencies))
					return false;
			}
		}
	}

	bundle.addDependencies(dependencies);
	bundle.sortDependencies();

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
bool AppBundler::runScriptTarget(const ScriptTarget& inScript)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	if (!inScript.description().empty())
		Output::msgScriptDescription(inScript.description());
	else
		Output::msgScript(inScript.name());

	Output::lineBreak();

	ScriptRunner scriptRunner(m_state.tools, m_buildFile, m_cleanOutput);
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
