/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct VSCodeExtensionAwarenessAdapter
{
	explicit VSCodeExtensionAwarenessAdapter(const bool inVsCodium);

	void initialize();

	bool vscodium() const noexcept;
	const std::string& programPath() const noexcept;
	const std::string& codePath() const noexcept;

	bool chaletExtensionInstalled() const noexcept;
	bool cppToolsExtensionInstalled() const noexcept;

private:
	bool installChaletExtension();

	std::string getProgramPath() const;
	std::string getCodePath() const;
	StringList getInstalledExtensions() const;

	std::string m_programPath;
	std::string m_codePath;

	bool m_vscodium = false;
	bool m_chaletExtensionInstalled = false;
	bool m_cppToolsExtensionInstalled = false;
};
}
