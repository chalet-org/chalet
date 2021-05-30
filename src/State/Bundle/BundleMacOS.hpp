/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_MACOS_HPP
#define CHALET_BUNDLE_MACOS_HPP

namespace chalet
{
struct BundleMacOS
{
	BundleMacOS() = default;

	bool validate();

	const std::string& bundleName() const noexcept;
	void setBundleName(const std::string& inValue);

	const std::string& bundleIdentifier() const noexcept;
	void setBundleIdentifier(const std::string& inValue);

	const std::string& icon() const noexcept;
	void setIcon(const std::string& inValue);

	const std::string& infoPropertyList() const noexcept;
	void setInfoPropertyList(const std::string& inValue);

	bool makeDmg() const noexcept;
	void setMakeDmg(const bool inValue) noexcept;

	const std::string& dmgBackground1x() const noexcept;
	void setDmgBackground1x(const std::string& inValue);

	const std::string& dmgBackground2x() const noexcept;
	void setDmgBackground2x(const std::string& inValue);

	const StringList& dylibs() const noexcept;
	void addDylibs(StringList& inList);
	void addDylib(std::string& inValue);

private:
	StringList m_dylibs;

	std::string m_bundleName;
	std::string m_bundleIdentifier;
	std::string m_icon;
	std::string m_infoPropertyList;

	std::string m_dmgBackground1x;
	std::string m_dmgBackground2x;

	bool m_makeDmg = false;
};
}

#endif // CHALET_BUNDLE_MACOS_HPP
