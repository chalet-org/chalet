/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "Builder/BinaryDependencyMap.hpp"
#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct BundleTarget;
struct ScriptDistTarget;
struct BundleArchiveTarget;
struct MacosDiskImageTarget;
struct WindowsNullsoftInstallerTarget;
struct CommandLineInputs;
struct StatePrototype;
struct IAppBundler;

struct AppBundler
{
	explicit AppBundler(const CommandLineInputs& inInputs, StatePrototype& inPrototype);

	bool runBuilds();

	bool run(const DistTarget& inTarget);

	bool gatherDependencies(const BundleTarget& inTarget, BuildState& inState);

private:
	bool runBundleTarget(IAppBundler& inBundler, BuildState& inState);
	bool runScriptTarget(const ScriptDistTarget& inTarget);
	bool runArchiveTarget(const BundleArchiveTarget& inTarget);
	bool runMacosDiskImageTarget(const MacosDiskImageTarget& inTarget);
	bool runWindowsNullsoftInstallerTarget(const WindowsNullsoftInstallerTarget& inTarget);

	bool isTargetNameValid(const IDistTarget& inTarget) const;
	bool isTargetNameValid(const IDistTarget& inTarget, const BuildState& inState, std::string& outName) const;

	void displayHeader(const std::string& inLabel, const IDistTarget& inTarget, const std::string& inName = std::string()) const;
	bool removeOldFiles(IAppBundler& inBundler);
	bool makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inFrameworksPath, const std::string& inResourcePath);
	BuildState* getBuildState(const std::string& inBuildConfiguration) const;

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;

	HeapDictionary<BuildState> m_states;
	BinaryDependencyMap m_dependencyMap;

	StringList m_removedDirs;
	StringList m_archives;

	std::string m_detectedArch;
};
}

#endif // CHALET_APP_BUNDLER_HPP
