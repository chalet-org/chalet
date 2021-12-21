/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP
#define CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct WindowsNullsoftInstallerTarget final : public IDistTarget
{
	explicit WindowsNullsoftInstallerTarget();

	virtual bool validate() final;

	const std::string& file() const noexcept;
	void setFile(std::string&& inValue);

	const StringList& pluginDirs() const noexcept;
	void addPluginDirs(StringList&& inList);
	void addPluginDir(std::string&& inValue);

	const StringList& defines() const noexcept;
	void addDefines(StringList&& inList);
	void addDefine(std::string&& inValue);

private:
	StringList m_pluginDirs;
	StringList m_defines;

	std::string m_file;
};
}

#endif // CHALET_WINDOWS_NULLSOFT_INSTALLER_TARGET_HPP
