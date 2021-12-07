/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleLinux.hpp"

#include "FileTemplates/PlatformFileTemplates.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool BundleLinux::validate()
{
	bool result = true;

	if (!m_icon.empty())
	{
		if (!String::endsWith({ ".png", ".svg" }, m_icon))
		{
			Diagnostic::error("bundle.linux.icon must end with '.png' or '.svg', but was '{}'.", m_icon);
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error("bundle.linux.icon '{}' was not found.", m_icon);
			result = false;
		}
	}

	if (!m_desktopEntry.empty())
	{
		if (!String::endsWith(".desktop", m_desktopEntry))
		{
			Diagnostic::error("bundle.linux.desktopEntry must end with '.desktop', but was '{}'.", m_desktopEntry);
			result = false;
		}
		else if (!Commands::pathExists(m_desktopEntry))
		{
			std::ofstream(m_desktopEntry) << PlatformFileTemplates::linuxDesktopEntry();
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& BundleLinux::icon() const noexcept
{
	return m_icon;
}

void BundleLinux::setIcon(std::string&& inValue)
{
	m_icon = std::move(inValue);
}

/*****************************************************************************/
const std::string& BundleLinux::desktopEntry() const noexcept
{
	return m_desktopEntry;
}

void BundleLinux::setDesktopEntry(std::string&& inValue)
{
	m_desktopEntry = std::move(inValue);
}
}
