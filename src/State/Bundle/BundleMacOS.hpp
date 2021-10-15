/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_MACOS_HPP
#define CHALET_BUNDLE_MACOS_HPP

#include "State/Bundle/MacOSBundleType.hpp"

namespace chalet
{
struct BundleMacOS
{
	BundleMacOS() = default;

	bool validate();

	MacOSBundleType bundleType() const noexcept;
	void setBundleType(std::string&& inName);
	bool isAppBundle() const noexcept;

	const std::string& bundleName() const noexcept;
	void setBundleName(const std::string& inValue);

	const std::string& icon() const noexcept;
	void setIcon(std::string&& inValue);

	const std::string& infoPropertyList() const noexcept;
	void setInfoPropertyList(std::string&& inValue);

	const std::string& infoPropertyListContent() const noexcept;
	void setInfoPropertyListContent(std::string&& inValue);

	bool makeDmg() const noexcept;
	void setMakeDmg(const bool inValue) noexcept;

	const std::string& dmgBackground1x() const noexcept;
	void setDmgBackground1x(std::string&& inValue);

	const std::string& dmgBackground2x() const noexcept;
	void setDmgBackground2x(std::string&& inValue);

private:
	MacOSBundleType getBundleTypeFromString(const std::string& inValue) const;

	std::string m_bundleName;
	std::string m_icon;
	std::string m_infoPropertyList;
	std::string m_infoPropertyListContent;

	std::string m_dmgBackground1x;
	std::string m_dmgBackground2x;

	MacOSBundleType m_bundleType = MacOSBundleType::None;

	bool m_makeDmg = false;
};
}

#endif // CHALET_BUNDLE_MACOS_HPP
