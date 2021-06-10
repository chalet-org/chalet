/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UNIVERSAL_BINARY_MACOS_HPP
#define CHALET_UNIVERSAL_BINARY_MACOS_HPP

#include "Bundler/AppBundler.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct CommandLineInputs;
class BuildState;

struct UniversalBinaryMacOS
{
	explicit UniversalBinaryMacOS(const CommandLineInputs& inInputs, BuildState& inState, const bool inInstallDependencies);

	bool run();

private:
	bool gatherDependencies(BuildState& inStateA, BuildState& inStateB, BuildState& inUniversalState);
	std::unique_ptr<BuildState> getIntermediateState(std::string_view inReplaceA, std::string_view inReplaceB) const;
	StringList getProjectFiles(const BuildState& inState) const;
	bool createUniversalBinaries(const BuildState& inStateA, const BuildState& inStateB, const BuildState& inUniversalState) const;
	bool bundleState(BuildState& inUniversalState);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	AppBundler m_bundler;

	bool m_installDependencies = false;
};
}

#endif // CHALET_UNIVERSAL_BINARY_MACOS_HPP
