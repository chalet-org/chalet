/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IAPP_BUNDLER_HPP
#define CHALET_IAPP_BUNDLER_HPP

#include "Builder/BinaryDependencyMap.hpp"

namespace chalet
{
class BuildState;
struct BundleTarget;

struct IAppBundler
{
	explicit IAppBundler(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);
	virtual ~IAppBundler() = default;

	const BundleTarget& bundle() const noexcept;
	bool getMainExecutable(std::string& outMainExecutable);
	StringList getAllExecutables() const;

	bool copyIncludedPath(const std::string& inDep, const std::string& inOutPath);
	const std::string& workingDirectoryWithTrailingPathSeparator();

	virtual bool removeOldFiles() = 0;
	virtual bool bundleForPlatform() = 0;
	virtual bool quickBundleForPlatform();

	virtual std::string getBundlePath() const = 0;
	virtual std::string getExecutablePath() const = 0;
	virtual std::string getResourcePath() const = 0;
	virtual std::string getFrameworksPath() const = 0;

	[[nodiscard]] static Unique<IAppBundler> make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

protected:
	BuildState& m_state;
	const BundleTarget& m_bundle;
	BinaryDependencyMap& m_dependencyMap;

private:
	std::string m_cwd;
};
}

#endif // CHALET_IAPP_BUNDLER_HPP
