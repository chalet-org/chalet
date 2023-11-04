/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_WINDOWS_HPP
#define CHALET_APP_BUNDLER_WINDOWS_HPP

#include "Bundler/IAppBundler.hpp"

namespace chalet
{
class BuildState;

class AppBundlerWindows : public IAppBundler
{
public:
	explicit AppBundlerWindows(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;
	virtual std::string getFrameworksPath() const final;

private:
	//
};
}

#endif // CHALET_APP_BUNDLER_WINDOWS_HPP
