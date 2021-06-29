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
	return true;
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
