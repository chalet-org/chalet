/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSTasksGen
{
	explicit VSTasksGen(const BuildState& inState);

	bool saveToFile(const std::string& inFilename);

private:
	const std::string& getToolchain() const;

	const BuildState& m_state;

	std::string m_toolchain;
	std::string m_architecture;
};
}
