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
// #include "Utility/Timer.hpp"

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
/*****************************************************************************/
AppBundler::AppBundler(BuildState& inState, const std::string& inBuildFile) :
	m_state(inState),
	m_buildFile(inBuildFile)
{
#if defined(CHALET_WIN32)
	UNUSED(inBuildFile);
	m_impl = std::make_unique<AppBundlerWindows>(inState);
#elif defined(CHALET_MACOS)
	m_impl = std::make_unique<AppBundlerMacOS>(inState, inBuildFile);
#elif defined(CHALET_LINUX)
	UNUSED(inBuildFile);
	m_impl = std::make_unique<AppBundlerLinux>(inState);
#else
	#error "Unknown platform"
#endif

	m_cleanOutput = m_state.environment.cleanOutput();
}

/*****************************************************************************/
bool AppBundler::run()
{
	if (!removeOldFiles())
	{
		Diagnostic::error("There was an error removing the previous distribution bundles");
		return false;
	}

	for (auto& target : m_state.distribution)
	{
		// Timer buildTimer;

		if (target->isDistributionBundle())
		{
			if (!runBundleTarget(static_cast<BundleTarget&>(*target)))
				return false;
		}
		else if (target->isScript())
		{
			if (!runScriptTarget(static_cast<const ScriptTarget&>(*target)))
				return false;
		}
		// Output::print(Color::Reset, fmt::format("   Time: {}", buildTimer.asString()));
		// Output::lineBreak();
	}

	return true;
}

bool AppBundler::runBundleTarget(BundleTarget& inBundle)
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& bundleProjects = inBundle.projects();

	const auto bundlePath = m_impl->getBundlePath(inBundle);
	const auto executablePath = m_impl->getExecutablePath(inBundle);
	const auto resourcePath = m_impl->getResourcePath(inBundle);

	makeBundlePath(bundlePath, executablePath, resourcePath);

	// auto path = Environment::getPath();
	// LOG(path);

	StringList dependencies;
	StringList executables;
#if defined(CHALET_MACOS)
	StringList sharedLibraries;
#endif

	StringList depsFromJson;
	for (auto& dep : inBundle.dependencies())
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

			const auto& filename = project.outputFile();
			const auto buildTarget = fmt::format("{}/{}", buildOutputDir, filename);

			if (project.isExecutable())
			{
				std::string outTarget = buildTarget;
				List::addIfDoesNotExist(executables, std::move(outTarget));
			}
			dependencies.push_back(buildTarget);

			if (!m_state.tools.getExecutableDependencies(buildTarget, dependencies))
				return false;
		}
	}

	inBundle.addDependencies(dependencies);
	inBundle.sortDependencies();

	uint copyCount = 0;
	for (auto& dep : inBundle.dependencies())
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

	Commands::forEachFileMatch(resourcePath, inBundle.excludes(), [this](const fs::path& inPath) {
		Commands::remove(inPath.string(), m_cleanOutput);
	});

	if (copyCount > 0)
	{
		Output::lineBreak();
	}

	if (!m_impl->bundleForPlatform(inBundle, m_cleanOutput))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::runScriptTarget(const ScriptTarget& inScript)
{
	const auto& scripts = inScript.scripts();
	if (scripts.empty())
		return false;

	// if (!inScript.description().empty())
	// 	Output::msgScriptDescription(inScript.description());
	// else
	// 	Output::msgScript(inScript.name());

	// Output::lineBreak();

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
bool AppBundler::removeOldFiles()
{
	for (auto& target : m_state.distribution)
	{
		if (target->isDistributionBundle())
		{
			auto& bundle = static_cast<const BundleTarget&>(*target);
			const auto& outDir = bundle.outDir();
			Commands::removeRecursively(outDir, m_cleanOutput);

			if (!m_impl->removeOldFiles(bundle, m_cleanOutput))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool AppBundler::makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath)
{
	// make prod dir
	if (!Commands::makeDirectory(inBundlePath, m_cleanOutput))
		return false;

#if defined(CHALET_MACOS)
	if (!Commands::makeDirectory(inExecutablePath, m_cleanOutput))
		return false;

	if (!Commands::makeDirectory(inResourcePath, m_cleanOutput))
		return false;
#else
	UNUSED(inExecutablePath, inResourcePath);
#endif
	return true;
}
}
