/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/Bundle/BundleWindows.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
// Something might fit here in the future, but for now, nothing!!
/*****************************************************************************/
bool BundleWindows::validate()
{
	bool result = true;

	if (!m_nsisScript.empty())
	{
		if (!String::endsWith("nsi", m_nsisScript))
		{
			Diagnostic::error("bundle.windows.nsisScript must end with '.nsi', but was '{}'.", m_nsisScript);
			result = false;
		}
		else if (!Commands::pathExists(m_nsisScript))
		{
			Diagnostic::error("bundle.windows.nsisScript '{}' was not found.", m_nsisScript);
			result = false;
		}
	}

	return result;
}

/*****************************************************************************/
const std::string& BundleWindows::nsisScript() const noexcept
{
	return m_nsisScript;
}

void BundleWindows::setNsisScript(std::string&& inValue)
{
	m_nsisScript = std::move(inValue);
}
}
