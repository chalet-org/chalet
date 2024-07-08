/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Bundler/IAppBundler.hpp"

namespace chalet
{
class BuildState;

class AppBundlerWindows : public IAppBundler
{
public:
	explicit AppBundlerWindows(BuildState& inState, const BundleTarget& inBundle, BinaryDependencyMap& inDependencyMap);

	virtual bool removeOldFiles() final;
	virtual bool bundleForPlatform() final;

private:
	//
};
}
