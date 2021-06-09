/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_UNIVERSAL_BINARY_MACOS_HPP
#define CHALET_UNIVERSAL_BINARY_MACOS_HPP

namespace chalet
{
struct CommandLineInputs;
class BuildState;

struct UniversalBinaryMacOS
{
	explicit UniversalBinaryMacOS(const CommandLineInputs& inInputs, BuildState& inState, const bool inInstallDependencies);

	bool run();

private:
	std::unique_ptr<BuildState> getIntermediateState(std::string_view inReplaceA, std::string_view inReplaceB) const;
	StringList getProjectFiles(const BuildState& inState) const;
	bool createUniversalBinaries(const BuildState& inStateA, const BuildState& inStateB, const BuildState& inUniversalState) const;
	bool bundleState(BuildState& inState) const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	bool m_installDependencies = false;
};
}

#endif // CHALET_UNIVERSAL_BINARY_MACOS_HPP
