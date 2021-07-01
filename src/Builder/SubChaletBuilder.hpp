/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SUB_CHALET_BUILDER_HPP
#define CHALET_SUB_CHALET_BUILDER_HPP

namespace chalet
{
class BuildState;
struct SubChaletTarget;
struct CommandLineInputs;

class SubChaletBuilder
{
public:
	explicit SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const CommandLineInputs& inInputs);

	bool run();

private:
	StringList getBuildCommand(const std::string& inLocation) const;

	const BuildState& m_state;
	const SubChaletTarget& m_target;
	const CommandLineInputs& m_inputs;

	std::string m_outputLocation;
	std::string m_buildFile;
};
}

#endif // CHALET_SUB_CHALET_BUILDER_HPP
