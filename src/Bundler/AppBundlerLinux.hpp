/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINUX_APP_BUNDLER_HPP
#define CHALET_LINUX_APP_BUNDLER_HPP

#include "Bundler/IAppBundler.hpp"
#include "Libraries/FileSystem.hpp"

namespace chalet
{
class BuildState;

class AppBundlerLinux : public IAppBundler
{
public:
	explicit AppBundlerLinux(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

private:
	std::string m_home;
	std::string m_applicationsPath;
};
}

#endif // CHALET_LINUX_APP_BUNDLER_HPP
