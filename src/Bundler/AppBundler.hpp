/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Distribution/IDistTarget.hpp"

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
struct SourceCache;

struct AppBundler
{
	explicit AppBundler(BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(AppBundler);
	~AppBundler();

	bool run(const DistTarget& inTarget);

	void reportErrors();

private:
	bool gatherDependencies(BundleTarget& inTarget);

	bool runBundleTarget(IAppBundler& inBundler);
	bool runArchiveTarget(const BundleArchiveTarget& inTarget);
	bool runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget);
	bool runScriptTarget(const ScriptDistTarget& inTarget);
	bool runProcessTarget(const ProcessDistTarget& inTarget);
	bool runValidationTarget(const ValidationDistTarget& inTarget);

	bool runProcess(const StringList& inCmd, std::string outputFile);

	bool isTargetNameValid(const IDistTarget& inTarget, std::string& outName) const;

	void displayHeader(const char* inLabel, const IDistTarget& inTarget, const std::string& inName = std::string()) const;
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
