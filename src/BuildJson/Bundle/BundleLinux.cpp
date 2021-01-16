/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildJson/Bundle/BundleLinux.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"

#include "Builder/PlatformFile.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& BundleLinux::icon() const noexcept
{
	return m_icon;
}

void BundleLinux::setIcon(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || path.extension() != ".png")
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.linux.icon) must be '.png'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.linux.icon) was not found. Aborting...", inValue));
		return;
	}

	m_icon = inValue;
}

/*****************************************************************************/
const std::string& BundleLinux::desktopEntry() const noexcept
{
	return m_desktopEntry;
}

void BundleLinux::setDesktopEntry(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || path.extension() != ".desktop")
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.linux.desktopEntry) must be '.desktop'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		std::ofstream(path) << PlatformFile::linuxDesktopEntry();
	}

	m_desktopEntry = inValue;
}
}
