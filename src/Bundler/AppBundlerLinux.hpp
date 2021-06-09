/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINUX_APP_BUNDLER_HPP
#define CHALET_LINUX_APP_BUNDLER_HPP

#include "Bundler/IAppBundler.hpp"
#include "Libraries/FileSystem.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class AppBundlerLinux : public IAppBundler
{
public:
	explicit AppBundlerLinux(BuildState& inState, BundleTarget& inBundle, const bool inCleanOutput);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

private:
	fs::path m_home;
	std::string m_applicationsPath;
	std::string m_mainExecutable;
};
}

#endif // CHALET_LINUX_APP_BUNDLER_HPP
