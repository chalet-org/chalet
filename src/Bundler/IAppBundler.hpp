/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IAPP_BUNDLER_HPP
#define CHALET_IAPP_BUNDLER_HPP

#include "Bundler/BinaryDependencyMap.hpp"

namespace chalet
{
class BuildState;
struct BundleTarget;

struct IAppBundler
{
	explicit IAppBundler(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const bool inCleanOutput);
	virtual ~IAppBundler() = default;

	const BundleTarget& bundle() const noexcept;

	virtual bool removeOldFiles() = 0;
	virtual bool bundleForPlatform() = 0;

	virtual std::string getBundlePath() const = 0;
	virtual std::string getExecutablePath() const = 0;
	virtual std::string getResourcePath() const = 0;

	[[nodiscard]] static std::unique_ptr<IAppBundler> make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap, const std::string& inBuildFile, const bool inCleanOutput);

protected:
	BuildState& m_state;
	const BundleTarget& m_bundle;
	BinaryDependencyMap& m_dependencyMap;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_IAPP_BUNDLER_HPP
