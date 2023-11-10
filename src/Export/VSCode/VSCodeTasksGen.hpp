/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSCodeTasksGen
{
	explicit VSCodeTasksGen(const BuildState& inState);

	bool saveToFile(const std::string& inFilename) const;

private:
	Json getTask() const;

	void setLabel(Json& outJson) const;
	std::string getProblemMatcher() const;

	bool willUseMSVC() const;

	const BuildState& m_state;
};
}
