/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleWindows.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string& BundleWindows::icon() const noexcept
{
	return m_icon;
}

void BundleWindows::setIcon(const std::string& inValue)
{
	fs::path path{ inValue };

	if (!path.has_extension() || path.extension() != ".ico")
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.windows.icon) must be '.ico'. Aborting...", inValue));
		return;
	}

	if (!Commands::pathExists(path))
	{
		Diagnostic::errorAbort(fmt::format("{} (bundle.windows.icon) was not found. Aborting...", inValue));
		return;
	}

	m_icon = inValue;
}

/*****************************************************************************/
const std::string& BundleWindows::manifest() const noexcept
{
	return m_manifest;
}

void BundleWindows::setManifest(const std::string& inValue)
{
	m_manifest = inValue;
}
}
