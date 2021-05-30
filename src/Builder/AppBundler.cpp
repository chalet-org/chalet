/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/AppBundler.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

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
	m_state(inState)
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
	auto& bundle = m_state.bundle;

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	const auto& bundleProjects = bundle.projects();

	const auto bundlePath = m_impl->getBundlePath();
	const auto executablePath = m_impl->getExecutablePath();
	const auto resourcePath = m_impl->getResourcePath();

	removeOldFiles();
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
		Output::lineBreak();
	}

	if (!m_impl->bundleForPlatform(m_cleanOutput))
		return false;

	return true;
}

/*****************************************************************************/
bool AppBundler::removeOldFiles()
{
	const auto& outDir = m_state.bundle.outDir();
	if (!Commands::removeRecursively(outDir, m_cleanOutput))
		return false;

	if (!m_impl->removeOldFiles(m_cleanOutput))
		return false;

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
