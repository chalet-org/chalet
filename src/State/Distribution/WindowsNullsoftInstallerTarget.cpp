/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WindowsNullsoftInstallerTarget::WindowsNullsoftInstallerTarget() :
	IDistTarget(DistTargetType::WindowsNullsoftInstaller)
{
}

/*****************************************************************************/
bool WindowsNullsoftInstallerTarget::validate()
{
	bool result = true;

	if (!m_nsisScript.empty())
	{
		if (!String::endsWith("nsi", m_nsisScript))
		{
			Diagnostic::error("windowsNullsoftInstaller.nsisScript must end with '.nsi', but was '{}'.", m_nsisScript);
			result = false;
		}
		else if (!Commands::pathExists(m_nsisScript))
		{
			Diagnostic::error("windowsNullsoftInstaller.nsisScript '{}' was not found.", m_nsisScript);
			result = false;
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& WindowsNullsoftInstallerTarget::nsisScript() const noexcept
{
	return m_nsisScript;
}

void WindowsNullsoftInstallerTarget::setNsisScript(std::string&& inValue)
{
	m_nsisScript = std::move(inValue);
}

/*****************************************************************************/
const StringList& WindowsNullsoftInstallerTarget::pluginDirs() const noexcept
{
	return m_pluginDirs;
}

void WindowsNullsoftInstallerTarget::addPluginDirs(StringList&& inList)
{
	List::forEach(inList, this, &WindowsNullsoftInstallerTarget::addPluginDir);
}

void WindowsNullsoftInstallerTarget::addPluginDir(std::string&& inValue)
{
	List::addIfDoesNotExist(m_pluginDirs, std::move(inValue));
}
}
