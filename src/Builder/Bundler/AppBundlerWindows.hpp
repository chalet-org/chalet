/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_WINDOWS_APP_BUNDLER_HPP
#define CHALET_WINDOWS_APP_BUNDLER_HPP

#include "Builder/Bundler/IAppBundler.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class AppBundlerWindows : public IAppBundler
{
public:
	explicit AppBundlerWindows(BuildState& inState);

	virtual bool removeOldFiles(const bool inCleanOutput) final;
	virtual bool bundleForPlatform(const bool inCleanOutput) final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

private:
	BuildState& m_state;
};
}

#endif // CHALET_WINDOWS_APP_BUNDLER_HPP
