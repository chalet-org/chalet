/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLE_HPP
#define CHALET_APP_BUNDLE_HPP

#include "Compile/CompilerTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/Bundle/BundleLinux.hpp"
#include "State/Bundle/BundleMacOS.hpp"
#include "State/Bundle/BundleWindows.hpp"
#include "State/Target/ProjectTarget.hpp"

namespace chalet
{
struct AppBundle
{
	explicit AppBundle(const BuildEnvironment& inEnvironment, const BuildTargetList& inTargets, const BuildPaths& inPaths, const CompilerTools& inCompilers);

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

	const std::string& outDir() const noexcept;
	void setOutDir(const std::string& inValue);

	const std::string& configuration() const noexcept;
	void setConfiguration(const std::string& inValue);

	bool exists() const noexcept;
	void setExists(const bool inValue) noexcept;

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
	const BuildEnvironment& m_environment;
	const BuildTargetList& m_targets;
	const BuildPaths& m_paths;
	const CompilerTools& m_compilers;

	BundleLinux m_linuxBundle;
	BundleMacOS m_macosBundle;
	BundleWindows m_windowsBundle;

	StringList m_projects;
	StringList m_dependencies;
	StringList m_excludes;

	std::string m_appName;
	std::string m_shortDescription;
	std::string m_longDescription;
	std::string m_distDir{ "dist" };
	std::string m_configuration;

	bool m_exists = true;
};
}

#endif // CHALET_APP_BUNDLE_HPP
