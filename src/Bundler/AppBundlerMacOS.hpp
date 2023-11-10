/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Bundler/IAppBundler.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
class BuildState;

class AppBundlerMacOS : public IAppBundler
{
public:
	explicit AppBundlerMacOS(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

	bool initializeState();

	void setOutputDirectory(const std::string& inPath) const;

	const std::string& mainExecutable() const noexcept;

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;
	virtual bool quickBundleForPlatform() final;

	virtual std::string getBundlePath() const final;
	virtual std::string getExecutablePath() const final;
	virtual std::string getResourcePath() const final;
	virtual std::string getFrameworksPath() const final;

	bool changeRPathOfDependents(const std::string& inInstallNameTool, const BinaryDependencyMap& inDependencyMap, const std::string& inExecutablePath) const;
	bool changeRPathOfDependents(const std::string& inInstallNameTool, const std::string& inFile, const StringList& inDependencies, const std::string& inOutputFile) const;

	bool createAssetsXcassets(const std::string& inOutPath);
	bool createInfoPropertyListAndReplaceVariables(const std::string& inOutFile, Json* outJson = nullptr) const;
	bool createEntitlementsPropertyList(const std::string& inOutFile) const;

private:
	std::string getPlistFile() const;
	std::string getEntitlementsFilePath() const;

	bool createBundleIcon();
	bool createBundleIconFromXcassets();
	bool setExecutablePaths() const;
	bool signAppBundle() const;

	mutable std::string m_outputDirectory;

	std::string m_mainExecutable;

	StringList m_executableOutputPaths;

	std::string m_executablePath;
	std::string m_bundlePath;
	std::string m_frameworksPath;
	std::string m_resourcePath;
	std::string m_iconBaseName;

	std::string m_infoFile;
	std::string m_entitlementsFile;
};
}
