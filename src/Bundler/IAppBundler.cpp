/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/IAppBundler.hpp"

namespace chalet
{
/*****************************************************************************/
IAppBundler::IAppBundler(BuildState& inState, BundleTarget& inBundle, const bool inCleanOutput) :
	m_state(inState),
	m_bundle(inBundle),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
BundleTarget& IAppBundler::bundle() const noexcept
{
	return m_bundle;
}
}
