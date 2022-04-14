/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
class BuildState;
struct BundleTarget;
struct ScriptDistTarget;
struct ProcessDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct WindowsNullsoftInstallerTarget;
class BinaryDependencyMap;
struct IAppBundler;

struct AppBundler
{
	explicit AppBundler(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(AppBundler);
	~AppBundler();

	// bool runBuilds();

	bool run(const DistTarget& inTarget);

	bool gatherDependencies(const BundleTarget& inTarget);

	void reportErrors() const;

private:
	bool runBundleTarget(IAppBundler& inBundler);
	bool runScriptTarget(const ScriptDistTarget& inTarget);
	bool runProcessTarget(const ProcessDistTarget& inTarget);
	bool runArchiveTarget(const BundleArchiveTarget& inTarget);
	bool runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget);
	bool runWindowsNullsoftInstallerTarget(const WindowsNullsoftInstallerTarget& inTarget);

	bool runProcess(const StringList& inCmd, std::string outputFile);

	bool isTargetNameValid(const IDistTarget& inTarget) const;
	bool isTargetNameValid(const IDistTarget& inTarget, std::string& outName) const;

	void displayHeader(const std::string& inLabel, const IDistTarget& inTarget, const std::string& inName = std::string()) const;
	bool removeOldFiles(IAppBundler& inBundler);
	bool makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inFrameworksPath, const std::string& inResourcePath);

	BuildState& m_state;

	Unique<BinaryDependencyMap> m_dependencyMap;

	StringList m_removedDirs;
	StringList m_archives;
	StringList m_notCopied;

	std::string m_detectedArch;
};
}

#endif // CHALET_APP_BUNDLER_HPP
