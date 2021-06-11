/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_LINUX_HPP
#define CHALET_BUNDLE_LINUX_HPP

namespace chalet
{
struct BundleLinux
{
	BundleLinux() = default;

	bool validate();

	const std::string& icon() const noexcept;
	void setIcon(std::string&& inValue);

	const std::string& desktopEntry() const noexcept;
	void setDesktopEntry(std::string&& inValue);

private:
	std::string m_icon;
	std::string m_desktopEntry;
};
}

#endif // CHALET_BUNDLE_LINUX_HPP
