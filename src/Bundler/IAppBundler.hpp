/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Bundler/BinaryDependency/BinaryDependencyMap.hpp"

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

	virtual std::string getBundlePath() const;
	virtual std::string getExecutablePath() const;
	virtual std::string getResourcePath() const;
	virtual std::string getFrameworksPath() const;

	[[nodiscard]] static Unique<IAppBundler> make(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

protected:
	BuildState& m_state;
	const BundleTarget& m_bundle;
	BinaryDependencyMap& m_dependencyMap;

private:
	std::string m_cwd;
};
}
