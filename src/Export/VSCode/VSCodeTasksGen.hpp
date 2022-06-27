/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VSCODE_TASKS_GEN_HPP
#define CHALET_VSCODE_TASKS_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSCodeTasksGen
{
	explicit VSCodeTasksGen(const BuildState& inState, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename) const;

private:
	Json getTask() const;

	void setLabel(Json& outJson) const;
	std::string getProblemMatcher() const;

	bool willUseMSVC() const;

	const BuildState& m_state;
	const std::string& m_cwd;
};
}

#endif // CHALET_VSCODE_LAUNCH_GEN_HPP
