/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_TARGET_HPP
#define CHALET_BUNDLE_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"
#include "State/MacOSBundleType.hpp"

namespace chalet
{
class BuildState;

struct BundleTarget final : public IDistTarget
{
	explicit BundleTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	bool resolveIncludesFromState(const BuildState& inState);

	bool updateRPaths() const noexcept;
	void setUpdateRPaths(const bool inValue) noexcept;

	const std::string& subdirectory() const noexcept;
	void setSubdirectory(std::string&& inValue);

	const std::string& mainExecutable() const noexcept;
	void setMainExecutable(std::string&& inValue);

	bool includeDependentSharedLibraries() const noexcept;
	void setIncludeDependentSharedLibraries(const bool inValue) noexcept;

	const StringList& buildTargets() const noexcept;
	void addBuildTargets(StringList&& inList);
	void addBuildTarget(std::string&& inValue);
	bool hasAllBuildTargets() const noexcept;

	const StringList& excludes() const noexcept;
	void addExcludes(StringList&& inList);
	void addExclude(std::string&& inValue);

	const StringList& includes() const noexcept;
	void addIncludes(StringList&& inList);
	void addInclude(std::string&& inValue);

#if defined(CHALET_MACOS)
	MacOSBundleType macosBundleType() const noexcept;
	void setMacosBundleType(std::string&& inName);
	bool isMacosBundle() const noexcept;
	bool isMacosAppBundle() const noexcept;

	const std::string& macosBundleExtension() const noexcept;

	const std::string& macosBundleName() const noexcept;
	void setMacosBundleName(const std::string& inValue);

	const std::string& macosBundleIcon() const noexcept;
	void setMacosBundleIcon(std::string&& inValue);

	const std::string& macosBundleInfoPropertyList() const noexcept;
	void setMacosBundleInfoPropertyList(std::string&& inValue);

	const std::string& macosBundleInfoPropertyListContent() const noexcept;
	void setMacosBundleInfoPropertyListContent(std::string&& inValue);

	const std::string& macosBundleEntitlementsPropertyList() const noexcept;
	void setMacosBundleEntitlementsPropertyList(std::string&& inValue);

	const std::string& macosBundleEntitlementsPropertyListContent() const noexcept;
	void setMacosBundleEntitlementsPropertyListContent(std::string&& inValue);
#elif defined(CHALET_LINUX)
	const std::string& linuxDesktopEntryIcon() const noexcept;
	void setLinuxDesktopEntryIcon(std::string&& inValue);

	const std::string& linuxDesktopEntryTemplate() const noexcept;
	void setLinuxDesktopEntryTemplate(std::string&& inValue);

	bool hasLinuxDesktopEntry() const noexcept;
#endif

private:
	StringList m_buildTargets;
	StringList m_rawIncludes;
	StringList m_includes;
	StringList m_excludes;

	std::string m_subdirectory;
	std::string m_configuration;
	std::string m_mainExecutable;

#if defined(CHALET_MACOS)
	std::string m_macosBundleName;
	std::string m_macosBundleExtension;
	std::string m_macosBundleIcon;
	std::string m_macosBundleInfoPropertyList;
	std::string m_macosBundleInfoPropertyListContent;
	std::string m_macosBundleEntitlementsPropertyList;
	std::string m_macosBundleEntitlementsPropertyListContent;

	MacOSBundleType m_macosBundleType = MacOSBundleType::None;
#elif defined(CHALET_LINUX)
	std::string m_linuxDesktopEntryTemplate;
	std::string m_linuxDesktopEntryIcon;
#endif

	bool m_hasAllBuildTargets = false;
	bool m_includeDependentSharedLibraries = true;
	bool m_includesResolved = false;
	bool m_updateRPaths = true;
};
}

#endif // CHALET_BUNDLE_TARGET_HPP
