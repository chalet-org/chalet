/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_TARGET_HPP
#define CHALET_BUNDLE_TARGET_HPP

#include "State/Bundle/BundleLinux.hpp"
#include "State/Bundle/BundleMacOS.hpp"
#include "State/Bundle/BundleWindows.hpp"
#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct BundleTarget final : public IDistTarget
{
	explicit BundleTarget();

	virtual void initialize(const BuildState& inState) final;
	virtual bool validate() final;

	bool updateRPaths() const noexcept;
	void setUpdateRPaths(const bool inValue) noexcept;

	const BundleLinux& linuxBundle() const noexcept;
	void setLinuxBundle(BundleLinux&& inValue);

	const BundleMacOS& macosBundle() const noexcept;
	void setMacosBundle(BundleMacOS&& inValue);

	const BundleWindows& windowsBundle() const noexcept;
	void setWindowsBundle(BundleWindows&& inValue);

	const std::string& outDir() const noexcept;
	void setOutDir(std::string&& inValue);

	const std::string& configuration() const noexcept;
	void setConfiguration(std::string&& inValue);

	const std::string& mainProject() const noexcept;
	void setMainProject(std::string&& inValue);

	bool includeDependentSharedLibraries() const noexcept;
	void setIncludeDependentSharedLibraries(const bool inValue) noexcept;

	const StringList& projects() const noexcept;
	void addProjects(StringList&& inList);
	void addProject(std::string&& inValue);

	const StringList& excludes() const noexcept;
	void addExcludes(StringList&& inList);
	void addExclude(std::string&& inValue);

	const StringList& dependencies() const noexcept;
	void addDependencies(StringList&& inList);
	void addDependency(std::string&& inValue);
	void sortDependencies();

private:
	void initializeDependencies(const BuildState& inState);

	BundleLinux m_linuxBundle;
	BundleMacOS m_macosBundle;
	BundleWindows m_windowsBundle;

	StringList m_projects;
	StringList m_rawDependencies;
	StringList m_dependencies;
	StringList m_excludes;

	std::string m_outDir;
	std::string m_configuration;
	std::string m_mainProject;

	bool m_includeDependentSharedLibraries = true;
	bool m_dependenciesResolved = false;
	bool m_updateRPaths = true;
};
}

#endif // CHALET_BUNDLE_TARGET_HPP
