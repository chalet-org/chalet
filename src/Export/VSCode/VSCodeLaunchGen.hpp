/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VSCODE_LAUNCH_GEN_HPP
#define CHALET_VSCODE_LAUNCH_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct VSCodeLaunchGen
{
	explicit VSCodeLaunchGen(const BuildState& inState, const IBuildTarget& inTarget);

	bool saveToFile(const std::string& inFilename) const;

private:
	Json getConfiguration() const;
	std::string getName() const;
	std::string getType() const;
	std::string getDebuggerPath() const;
	void setOptions(Json& outJson) const;
	void setPreLaunchTask(Json& outJson) const;
	void setProgramPath(Json& outJson) const;
	void setEnvFilePath(Json& outJson) const;

	bool willUseMSVC() const;
	bool willUseLLDB() const;
	bool willUseGDB() const;

	const BuildState& m_state;
	const IBuildTarget& m_target;
};
}

#endif // CHALET_VSCODE_LAUNCH_GEN_HPP
