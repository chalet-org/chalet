/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINUX_APP_BUNDLER_HPP
#define CHALET_LINUX_APP_BUNDLER_HPP

#include "Builder/Bundler/IAppBundler.hpp"
#include "Libraries/FileSystem.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class AppBundlerLinux : public IAppBundler
{
public:
	explicit AppBundlerLinux(BuildState& inState);

	virtual bool removeOldFiles(const bool inCleanOutput) final;
	virtual bool bundleForPlatform(const bool inCleanOutput) final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

private:
	BuildState& m_state;

	fs::path m_home;
	std::string m_applicationsPath;
};
}

#endif // CHALET_LINUX_APP_BUNDLER_HPP
