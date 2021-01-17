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
AppBundler::AppBundler(BuildState& inState) :
	m_state(inState)
{
#if defined(CHALET_WIN32)
	m_impl = std::make_unique<AppBundlerWindows>(inState);
#elif defined(CHALET_MACOS)
	m_impl = std::make_unique<AppBundlerMacOS>(inState);
#elif defined(CHALET_LINUX)
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

	const auto& buildDir = m_state.paths.buildDir();
	const auto& bundleProjects = bundle.projects();

	const std::string bundlePath = m_impl->getBundlePath();
	const std::string executablePath = m_impl->getExecutablePath();
	const std::string resourcePath = m_impl->getResourcePath();

	removeOldFiles();
	makeBundlePath(bundlePath, executablePath, resourcePath);

	// auto path = Environment::getPath();
	// LOG(path);

	bool result = true;

	StringList dependencies;
	StringList executables;

	for (auto& project : m_state.projects)
	{
		if (!project->includeInBuild())
			continue;

		if (!List::contains(bundleProjects, project->name()))
			continue;

		const auto& filename = project->outputFile();
		const std::string target = fmt::format("{buildDir}/{filename}",
			FMT_ARG(buildDir),
			FMT_ARG(filename));

		if (project->isExecutable())
		{
			std::string outTarget = target;
			List::addIfDoesNotExist(executables, std::move(outTarget));
		}

		dependencies.push_back(target);

		if (!m_state.tools.getExecutableDependencies(target, dependencies))
			return false;
	}

	bundle.addDependencies(dependencies);
	bundle.sortDependencies();

	uint copyCount = 0;
	for (auto& dep : bundle.dependencies())
	{
		result &= Commands::copy(dep, executablePath, m_cleanOutput);
		++copyCount;

#if !defined(CHALET_WIN32)
		if (List::contains(executables, dep))
		{
			const std::string filename = String::getPathFilename(dep);
			const std::string executable = fmt::format("{}/{}", executablePath, filename);
			result &= Commands::setExecutableFlag(executable, m_cleanOutput);
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

	m_impl->bundleForPlatform(m_cleanOutput);

	return result;
}

/*****************************************************************************/
bool AppBundler::removeOldFiles()
{
	const auto& distFolder = m_state.bundle.path();
	bool result = Commands::removeRecursively(distFolder, m_cleanOutput);

	result &= m_impl->removeOldFiles(m_cleanOutput);

	return result;
}

/*****************************************************************************/
bool AppBundler::makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath)
{
	// make prod dir
	bool result = true;
	result &= Commands::makeDirectory(inBundlePath, m_cleanOutput);
#if defined(CHALET_MACOS)
	result &= Commands::makeDirectory(inExecutablePath, m_cleanOutput);
	result &= Commands::makeDirectory(inResourcePath, m_cleanOutput);
#else
	UNUSED(inExecutablePath, inResourcePath);
#endif
	return result;
}
}
