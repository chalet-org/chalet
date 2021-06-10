/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "Bundler/BinaryDependencyMap.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct BundleTarget;
struct ScriptTarget;
struct IAppBundler;

struct AppBundler
{
	AppBundler() = default;

	bool run(BuildTarget& inTarget, BuildState& inState, const std::string& inBuildFile);

	const BinaryDependencyMap& dependencyMap() const noexcept;
	bool gatherDependencies(BundleTarget& inTarget, BuildState& inState);
	void addDependencies(std::string&& inFile, StringList&& inDependencies);
	void logDependencies() const;

private:
	bool runBundleTarget(IAppBundler& inBundler, BuildState& inState);
	bool runScriptTarget(const ScriptTarget& inScript, BuildState& inState, const std::string& inBuildFile);
	bool removeOldFiles(IAppBundler& inBundler);
	bool makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath);

	BinaryDependencyMap m_dependencyMap;

	StringList m_removedDirs;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_APP_BUNDLER_HPP
