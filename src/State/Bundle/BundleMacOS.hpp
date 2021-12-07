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

	const std::string& bundleExtension() const noexcept;

	const std::string& bundleName() const noexcept;
	void setBundleName(const std::string& inValue);

	const std::string& icon() const noexcept;
	void setIcon(std::string&& inValue);

	const std::string& infoPropertyList() const noexcept;
	void setInfoPropertyList(std::string&& inValue);

	const std::string& infoPropertyListContent() const noexcept;
	void setInfoPropertyListContent(std::string&& inValue);

private:
	MacOSBundleType getBundleTypeFromString(const std::string& inValue) const;

	std::string m_bundleName;
	std::string m_bundleExtension;
	std::string m_icon;
	std::string m_infoPropertyList;
	std::string m_infoPropertyListContent;

	MacOSBundleType m_bundleType = MacOSBundleType::None;
};
}

#endif // CHALET_BUNDLE_MACOS_HPP
