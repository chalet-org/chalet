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

	virtual bool initialize(const BuildState& inState) final;
	virtual bool validate() final;

	bool updateRPaths() const noexcept;
	void setUpdateRPaths(const bool inValue) noexcept;

	const BundleLinux& linuxBundle() const noexcept;
	void setLinuxBundle(BundleLinux&& inValue);

	const BundleMacOS& macosBundle() const noexcept;
	void setMacosBundle(BundleMacOS&& inValue);

	const BundleWindows& windowsBundle() const noexcept;
	void setWindowsBundle(BundleWindows&& inValue);

	const std::string& subdirectory() const noexcept;
	void setSubdirectory(std::string&& inValue);

	const std::string& configuration() const noexcept;
	void setConfiguration(std::string&& inValue);

	const std::string& mainExecutable() const noexcept;
	void setMainExecutable(std::string&& inValue);

	bool includeDependentSharedLibraries() const noexcept;
	void setIncludeDependentSharedLibraries(const bool inValue) noexcept;

	const StringList& buildTargets() const noexcept;
	void addBuildTargets(StringList&& inList);
	void addBuildTarget(std::string&& inValue);

	const StringList& excludes() const noexcept;
	void addExcludes(StringList&& inList);
	void addExclude(std::string&& inValue);

	const StringList& includes() const noexcept;
	void addIncludes(StringList&& inList);
	void addInclude(std::string&& inValue);
	void sortIncludes();

private:
	bool resolveIncludesFromState(const BuildState& inState);

	BundleLinux m_linuxBundle;
	BundleMacOS m_macosBundle;
	BundleWindows m_windowsBundle;

	StringList m_buildTargets;
	StringList m_rawIncludes;
	StringList m_includes;
	StringList m_excludes;

	std::string m_subdirectory;
	std::string m_configuration;
	std::string m_mainExecutable;

	bool m_includeDependentSharedLibraries = true;
	bool m_includesResolved = false;
	bool m_updateRPaths = true;
};
}

#endif // CHALET_BUNDLE_TARGET_HPP
