/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VSCODE_LAUNCH_GEN_HPP
#define CHALET_VSCODE_LAUNCH_GEN_HPP

namespace chalet
{
class BuildState;

struct VSCodeLaunchGen
{
	explicit VSCodeLaunchGen(const BuildState& inState, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename) const;

private:
	const BuildState& m_state;
	const std::string& m_cwd;
};
}

#endif // CHALET_VSCODE_LAUNCH_GEN_HPP
