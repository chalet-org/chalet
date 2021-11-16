/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_APP_BUNDLER_HPP
#define CHALET_MACOS_APP_BUNDLER_HPP

#include "Bundler/IAppBundler.hpp"

namespace chalet
{
class BuildState;

class AppBundlerMacOS : public IAppBundler
{
public:
	explicit AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inInputFile);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;

	bool changeRPathOfDependents(const std::string& inInstallNameTool, const BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath) const;
	bool changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile) const;

private:
	std::string getFrameworksPath() const;

	bool createBundleIcon();
	bool createPListAndReplaceVariables() const;
	bool setExecutablePaths() const;
	bool createDmgImage() const;
	bool signAppBundle() const;
	bool signDmgImage(const std::string& inPath) const;

	const std::string& m_inputFile;

	std::string m_bundlePath;
	std::string m_frameworkPath;
	std::string m_resourcePath;
	std::string m_executablePath;
	std::string m_executableOutputPath;
	std::string m_iconBaseName;
};
}

#endif // CHALET_MACOS_APP_BUNDLER_HPP
