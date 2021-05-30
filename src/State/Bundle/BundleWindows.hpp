/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_BUNDLE_WINDOWS_HPP
#define CHALET_BUNDLE_WINDOWS_HPP

namespace chalet
{
struct BundleWindows
{
	BundleWindows() = default;

	bool validate();

	const std::string& icon() const noexcept;
	void setIcon(const std::string& inValue);

	const std::string& manifest() const noexcept;
	void setManifest(const std::string& inValue);

private:
	std::string m_icon;
	std::string m_manifest;
};
}

#endif // CHALET_BUNDLE_WINDOWS_HPP
