/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_APP_BUNDLER_HPP
#define CHALET_MACOS_APP_BUNDLER_HPP

#include "Builder/Bundler/IAppBundler.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class AppBundlerMacOS : public IAppBundler
{
public:
	explicit AppBundlerMacOS(BuildState& inState, const std::string& inBuildFile, BundleTarget& inBundle, const bool inCleanOutput);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

private:
	bool changeRPathOfDependents() const;
	bool createBundleIcon();
	bool createPListAndUpdateCommonKeys() const;
	bool setExecutablePaths() const;
	bool createDmgImage() const;

	const std::string& m_buildFile;

	std::string m_bundlePath;
	std::string m_frameworkPath;
	std::string m_resourcePath;
	std::string m_executablePath;
	std::string m_mainExecutable;
	std::string m_executableOutputPath;
	std::string m_iconBaseName;
};
}

#endif // CHALET_MACOS_APP_BUNDLER_HPP
