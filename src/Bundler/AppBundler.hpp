/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "Bundler/BinaryDependencyMap.hpp"
#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct BundleTarget;
struct ScriptDistTarget;
struct CommandLineInputs;
struct StatePrototype;
struct IAppBundler;

struct AppBundler
{
	using StateMap = std::unordered_map<std::string, std::unique_ptr<BuildState>>;

	explicit AppBundler(const CommandLineInputs& inInputs, StatePrototype& inPrototype);

	bool runBuilds();

	bool run(const DistributionTarget& inTarget);

	const BinaryDependencyMap& dependencyMap() const noexcept;
	bool gatherDependencies(const BundleTarget& inTarget, BuildState& inState);
	void addDependencies(std::string&& inFile, StringList&& inDependencies);
	void logDependencies() const;

private:
	bool runBundleTarget(IAppBundler& inBundler, BuildState& inState);
	bool runScriptTarget(const ScriptDistTarget& inScript, const std::string& inInputFile);
	bool removeOldFiles(IAppBundler& inBundler);
	bool makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath);
	std::unique_ptr<BuildState> getUniversalState(BuildState& inState, const std::string& inBuildConfig) const;
	BuildState* getBuildState(const std::string& inBuildConfiguration) const;

	const CommandLineInputs& m_inputs;
	StatePrototype& m_prototype;

	StateMap m_states;
	BinaryDependencyMap m_dependencyMap;

	StringList m_removedDirs;

#if defined(CHALET_MACOS)
	std::unique_ptr<BuildState> m_univeralState;
#endif

	std::string m_detectedArch;
};
}

#endif // CHALET_APP_BUNDLER_HPP
