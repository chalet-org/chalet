/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_APP_BUNDLER_HPP
#define CHALET_MACOS_APP_BUNDLER_HPP

#include "Bundler/IAppBundler.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
class AppBundlerMacOS : public IAppBundler
{
public:
	explicit AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inBuildFile);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

	static bool changeRPathOfDependents(const std::string& inInstallNameTool, BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath);
	static bool changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile);

private:
	std::string getFrameworksPath() const;

	bool createBundleIcon();
	bool createPListAndUpdateCommonKeys() const;
	bool setExecutablePaths() const;
	bool createDmgImage() const;
	bool signAppBundle() const;

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
