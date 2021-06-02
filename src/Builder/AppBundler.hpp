/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_APP_BUNDLER_HPP
#define CHALET_APP_BUNDLER_HPP

#include "Builder/Bundler/IAppBundler.hpp"
#include "State/BuildState.hpp"
#include "State/Target/BundleTarget.hpp"
#include "State/Target/ScriptTarget.hpp"

namespace chalet
{
class AppBundler
{
public:
	explicit AppBundler(BuildState& inState, const std::string& inBuildFile);

	bool run();

private:
	bool runBundleTarget(BundleTarget& inBundle);
	bool runScriptTarget(const ScriptTarget& inScript);
	bool removeOldFiles();
	bool makeBundlePath(const std::string& inBundlePath, const std::string& inExecutablePath, const std::string& inResourcePath);

	std::unique_ptr<IAppBundler> m_impl;

	BuildState& m_state;
	const std::string& m_buildFile;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_APP_BUNDLER_HPP
