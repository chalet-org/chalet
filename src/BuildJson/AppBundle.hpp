/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLE_HPP
#define CHALET_APP_BUNDLE_HPP

#include "BuildJson/Bundle/BundleLinux.hpp"
#include "BuildJson/Bundle/BundleMacOS.hpp"
#include "BuildJson/Bundle/BundleWindows.hpp"
#include "BuildJson/CompileEnvironment.hpp"
#include "BuildJson/ProjectConfiguration.hpp"
#include "State/BuildPaths.hpp"

namespace chalet
{
struct AppBundle
{
	explicit AppBundle(const CompileEnvironment& inEnvironment, const ProjectConfigurationList& inProjectList, const BuildPaths& inPaths);

	const BundleLinux& linuxBundle() const noexcept;
	void setLinuxBundle(const BundleLinux& inValue);

	const BundleMacOS& macosBundle() const noexcept;
	void setMacosBundle(const BundleMacOS& inValue);

	const BundleWindows& windowsBundle() const noexcept;
	void setWindowsBundle(const BundleWindows& inValue);

	const std::string& appName() const noexcept;
	void setAppName(const std::string& inValue);

	const std::string& shortDescription() const noexcept;
	void setShortDescription(const std::string& inValue);

	const std::string& longDescription() const noexcept;
	void setLongDescription(const std::string& inValue);

	const std::string& path() const noexcept;
	void setPath(const std::string& inValue);

	const std::string& configuration() const noexcept;
	void setConfiguration(const std::string& inValue);

	const StringList& projects() const noexcept;
	void addProjects(StringList& inList);
	void addProject(std::string& inValue);

	const StringList& excludes() const noexcept;
	void addExcludes(StringList& inList);
	void addExclude(std::string& inValue);

	const StringList& dependencies() const noexcept;
	void addDependencies(StringList& inList);
	void addDependency(std::string& inValue);
	void sortDependencies();

private:
	const CompileEnvironment& m_environment;
	const ProjectConfigurationList& m_projectConfigs;
	const BuildPaths& m_paths;

	BundleLinux m_linuxBundle;
	BundleMacOS m_macosBundle;
	BundleWindows m_windowsBundle;

	StringList m_projects;
	StringList m_dependencies;
	StringList m_excludes;

	std::string m_appName;
	std::string m_shortDescription;
	std::string m_longDescription;
	std::string m_path{ "build" };
	std::string m_configuration;
};
}

#endif // CHALET_APP_BUNDLE_HPP
