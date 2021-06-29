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

	const std::string& nsisScript() const noexcept;
	void setNsisScript(std::string&& inValue);

	// nsisScript

private:
	std::string m_nsisScript;
};
}

#endif // CHALET_BUNDLE_WINDOWS_HPP
