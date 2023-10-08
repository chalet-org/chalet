/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "State/Distribution/IDistTarget.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
class BuildState;
struct BundleTarget;
struct ScriptDistTarget;
struct ProcessDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct ValidationDistTarget;
class BinaryDependencyMap;
struct IAppBundler;

struct AppBundler
{
	explicit AppBundler(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(AppBundler);
	~AppBundler();

	bool run(const DistTarget& inTarget);

	bool gatherDependencies(const BundleTarget& inTarget);

	void reportErrors();

private:
	bool runBundleTarget(IAppBundler& inBundler);
	bool runArchiveTarget(const BundleArchiveTarget& inTarget);
	bool runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget);
	bool runScriptTarget(const ScriptDistTarget& inTarget);
	bool runProcessTarget(const ProcessDistTarget& inTarget);
	bool runValidationTarget(const ValidationDistTarget& inTarget);

	bool runProcess(const StringList& inCmd, std::string outputFile);

	bool isTargetNameValid(const IDistTarget& inTarget) const;
	bool isTargetNameValid(const IDistTarget& inTarget, std::string& outName) const;

	void stopTimerAndShowBenchmark(Timer& outTimer);
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
