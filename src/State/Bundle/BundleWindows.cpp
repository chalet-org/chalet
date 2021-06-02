/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleWindows.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool BundleWindows::validate()
{
	bool result = true;

	if (!m_icon.empty())
	{
		if (!String::endsWith(".ico", m_icon))
		{
			Diagnostic::error(fmt::format("bundle.windows.icon must end with '.ico', but was '{}'.", m_icon));
			result = false;
		}
		else if (!Commands::pathExists(m_icon))
		{
			Diagnostic::error(fmt::format("bundle.windows.icon was not found.", m_icon));
			result = false;
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& BundleWindows::icon() const noexcept
{
	return m_icon;
}

void BundleWindows::setIcon(const std::string& inValue)
{
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
