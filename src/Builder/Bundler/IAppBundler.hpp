/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IAPP_BUNDLER_HPP
#define CHALET_IAPP_BUNDLER_HPP

#include "State/Target/BundleTarget.hpp"

namespace chalet
{
struct IAppBundler
{
	explicit IAppBundler(BuildState& inState, BundleTarget& inBundle, const bool inCleanOutput);
	virtual ~IAppBundler() = default;

	BundleTarget& bundle() const noexcept;

	virtual bool removeOldFiles() = 0;
	virtual bool bundleForPlatform() = 0;

	virtual std::string getBundlePath() const = 0;
	virtual std::string getExecutablePath() const = 0;
	virtual std::string getResourcePath() const = 0;

protected:
	BuildState& m_state;
	BundleTarget& m_bundle;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_IAPP_BUNDLER_HPP
