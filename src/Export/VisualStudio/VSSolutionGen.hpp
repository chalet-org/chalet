/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_SOLUTION_GEN_HPP
#define CHALET_VS_SOLUTION_GEN_HPP

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSSolutionGen
{
	explicit VSSolutionGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd);

	bool saveToFile(const std::string& inFilename);

private:
	const std::vector<Unique<BuildState>>& m_states;
	const std::string& m_cwd;
};
}

#endif // CHALET_VS_SOLUTION_GEN_HPP
