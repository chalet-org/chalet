/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/AppBundle.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
AppBundle::AppBundle(const BuildEnvironment& inEnvironment, const ProjectConfigurationList& inProjectList, const BuildPaths& inPaths, const CompilerCache& inCompilers) :
	m_environment(inEnvironment),
	m_projectConfigs(inProjectList),
	m_paths(inPaths),
	m_compilers(inCompilers)
{
}

/*****************************************************************************/
const BundleLinux& AppBundle::linuxBundle() const noexcept
{
	return m_linuxBundle;
}

void AppBundle::setLinuxBundle(const BundleLinux& inValue)
{
	m_linuxBundle = inValue;
}

/*****************************************************************************/
const BundleMacOS& AppBundle::macosBundle() const noexcept
{
	return m_macosBundle;
}

void AppBundle::setMacosBundle(const BundleMacOS& inValue)
{
	m_macosBundle = inValue;
}

/*****************************************************************************/
const BundleWindows& AppBundle::windowsBundle() const noexcept
{
	return m_windowsBundle;
}

void AppBundle::setWindowsBundle(const BundleWindows& inValue)
{
	m_windowsBundle = inValue;
}

/*****************************************************************************/
const std::string& AppBundle::appName() const noexcept
{
	return m_appName;
}

void AppBundle::setAppName(const std::string& inValue)
{
	m_appName = inValue;
}

/*****************************************************************************/
const std::string& AppBundle::shortDescription() const noexcept
{
	return m_shortDescription;
}

void AppBundle::setShortDescription(const std::string& inValue)
{
	m_shortDescription = inValue;
}

/*****************************************************************************/
const std::string& AppBundle::longDescription() const noexcept
{
	return m_longDescription;
}

void AppBundle::setLongDescription(const std::string& inValue)
{
	m_longDescription = inValue;
}

/*****************************************************************************/
const std::string& AppBundle::path() const noexcept
{
	return m_path;
}

void AppBundle::setPath(const std::string& inValue)
{
	m_path = inValue;
	Path::sanitize(m_path);
}

/*****************************************************************************/
const std::string& AppBundle::configuration() const noexcept
{
	return m_configuration;
}

void AppBundle::setConfiguration(const std::string& inValue)
{
	m_configuration = inValue;
}

/*****************************************************************************/
const StringList& AppBundle::projects() const noexcept
{
	return m_projects;
}

void AppBundle::addProjects(StringList& inList)
{
	List::forEach(inList, this, &AppBundle::addProject);
}

void AppBundle::addProject(std::string& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_projects, std::move(inValue));
}

/*****************************************************************************/
const StringList& AppBundle::excludes() const noexcept
{
	return m_excludes;
}

void AppBundle::addExcludes(StringList& inList)
{
	List::forEach(inList, this, &AppBundle::addExclude);
}

void AppBundle::addExclude(std::string& inValue)
{
	Path::sanitize(inValue);
	List::addIfDoesNotExist(m_excludes, std::move(inValue));
}

/*****************************************************************************/
const StringList& AppBundle::dependencies() const noexcept
{
	return m_dependencies;
}

void AppBundle::addDependencies(StringList& inList)
{
	List::forEach(inList, this, &AppBundle::addDependency);
}

void AppBundle::addDependency(std::string& inValue)
{
	const auto add = [this](std::string& in) {
		Path::sanitize(in);
		List::addIfDoesNotExist(m_dependencies, std::move(in));
	};

	if (Commands::pathExists(inValue))
	{
		add(inValue);
		return;
	}

	std::string resolved = fmt::format("{}/{}", m_paths.buildDir(), inValue);
	if (Commands::pathExists(resolved))
	{
		add(resolved);
		return;
	}

	for (auto& project : m_projectConfigs)
	{
		const auto& compilerConfig = m_compilers.getConfig(project->language());
		const auto& compilerPathBin = compilerConfig.compilerPathBin();

		resolved = fmt::format("{}/{}", compilerPathBin, inValue);
		if (Commands::pathExists(resolved))
		{
			add(resolved);
			return;
		}

		// LOG(resolved, " ", project->outputFile());
		if (String::contains(project->outputFile(), resolved))
		{
			add(resolved);
			return;
		}
	}

	for (auto& path : m_environment.path())
	{
		resolved = fmt::format("{}/{}", path, inValue);
		if (Commands::pathExists(resolved))
		{
			add(resolved);
			return;
		}
	}
}

/*****************************************************************************/
void AppBundle::sortDependencies()
{
	List::sort(m_dependencies);
}

}
