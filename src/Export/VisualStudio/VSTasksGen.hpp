/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_TASKS_GEN_HPP
#define CHALET_VS_TASKS_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct VSTasksGen
{
	explicit VSTasksGen(const BuildState& inState, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename);

private:
	const BuildState& m_state;
	const std::string& m_cwd;
};
}

#endif // CHALET_VS_TASKS_GEN_HPP
