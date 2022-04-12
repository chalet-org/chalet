/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"

#include "State/CentralState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
WindowsNullsoftInstallerTarget::WindowsNullsoftInstallerTarget(const CentralState& inCentralState) :
	IDistTarget(inCentralState, DistTargetType::WindowsNullsoftInstaller)
{
}

/*****************************************************************************/
bool WindowsNullsoftInstallerTarget::initialize()
{
	m_centralState.replaceVariablesInPath(m_file, this);

	replaceVariablesInPathList(m_defines);
	replaceVariablesInPathList(m_pluginDirs);

	return true;
}

/*****************************************************************************/
bool WindowsNullsoftInstallerTarget::validate()
{
	bool result = true;

	if (!m_file.empty())
	{
		if (!String::endsWith("nsi", m_file))
		{
			Diagnostic::error("windowsNullsoftInstaller.file must end with '.nsi', but was '{}'.", m_file);
			result = false;
		}
		else if (!Commands::pathExists(m_file))
		{
			Diagnostic::error("windowsNullsoftInstaller.file '{}' was not found.", m_file);
			result = false;
		}
	}
	else
	{
		Diagnostic::error("Nullsoft script file not found for target: {}", this->name());
		result = false;
	}

	return result;
}

/*****************************************************************************/
const std::string& WindowsNullsoftInstallerTarget::file() const noexcept
{
	return m_file;
}

void WindowsNullsoftInstallerTarget::setFile(std::string&& inValue)
{
	m_file = std::move(inValue);
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

/*****************************************************************************/
const StringList& WindowsNullsoftInstallerTarget::defines() const noexcept
{
	return m_defines;
}
void WindowsNullsoftInstallerTarget::addDefines(StringList&& inList)
{
	List::forEach(inList, this, &WindowsNullsoftInstallerTarget::addDefine);
}
void WindowsNullsoftInstallerTarget::addDefine(std::string&& inValue)
{
	List::addIfDoesNotExist(m_defines, std::move(inValue));
}

}
