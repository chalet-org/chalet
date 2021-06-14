/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UNIVERSAL_BINARY_MACOS_HPP
#define CHALET_UNIVERSAL_BINARY_MACOS_HPP

#include "Bundler/AppBundler.hpp"

namespace chalet
{
struct CommandLineInputs;
struct StatePrototype;
class BuildState;
struct BundleTarget;

struct UniversalBinaryMacOS
{
	explicit UniversalBinaryMacOS(AppBundler& inBundler, BuildState& inState, BundleTarget& inBundle);

	bool run(BuildState& inStateB, BuildState& inUniversalState);

private:
	bool gatherDependencies(BuildState& inStateA, BuildState& inStateB, BuildState& inUniversalState);
	StringList getProjectFiles(const BuildState& inState) const;
	bool createUniversalBinaries(const BuildState& inStateA, const BuildState& inStateB, const BuildState& inUniversalState) const;

	AppBundler& m_bundler;
	BuildState& m_state;
	BundleTarget& m_bundle;
};
}

#endif // CHALET_UNIVERSAL_BINARY_MACOS_HPP
