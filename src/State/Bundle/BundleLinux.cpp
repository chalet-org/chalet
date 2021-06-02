/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleLinux.hpp"

#include "Builder/PlatformFile.hpp"
#include "Libraries/Format.hpp"
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
		if (!String::endsWith(".png", m_icon))
		{
			Diagnostic::error(fmt::format("bundle.linux.icon must end with '.png', but was '{}'.", m_icon));
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error("bundle.linux.icon was not found.");
			result = false;
		}
	}

	if (!m_desktopEntry.empty())
	{
		if (!String::endsWith(".desktop", m_desktopEntry))
		{
			Diagnostic::error(fmt::format("bundle.linux.desktopEntry must end with '.desktop', but was '{}'.", m_desktopEntry));
			result = false;
		}
		else if (!Commands::pathExists(m_desktopEntry))
		{
			std::ofstream(m_desktopEntry) << PlatformFile::linuxDesktopEntry();
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& BundleLinux::icon() const noexcept
{
	return m_icon;
}

void BundleLinux::setIcon(const std::string& inValue)
{
	m_icon = inValue;
}

/*****************************************************************************/
const std::string& BundleLinux::desktopEntry() const noexcept
{
	return m_desktopEntry;
}

void BundleLinux::setDesktopEntry(const std::string& inValue)
{
	m_desktopEntry = inValue;
}
}
