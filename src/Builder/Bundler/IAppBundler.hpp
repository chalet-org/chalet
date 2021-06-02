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
	virtual ~IAppBundler() = default;

	virtual bool removeOldFiles(const BundleTarget& bundle, const bool inCleanOutput) = 0;
	virtual bool bundleForPlatform(const BundleTarget& bundle, const bool inCleanOutput) = 0;

	virtual std::string getBundlePath(const BundleTarget& bundle) const = 0;
	virtual std::string getExecutablePath(const BundleTarget& bundle) const = 0;
	virtual std::string getResourcePath(const BundleTarget& bundle) const = 0;
};
}

#endif // CHALET_IAPP_BUNDLER_HPP
