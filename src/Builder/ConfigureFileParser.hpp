/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourceTarget;
struct IBuildTarget;

struct ConfigureFileParser
{
	explicit ConfigureFileParser(const BuildState& inState, const SourceTarget& inProject);

	bool run(const std::string& inOutputFolder);

private:
	std::string getReplaceValue(std::string inKey) const;
	std::string getReplaceValueFromSubString(const std::string& inKey, const bool isTarget = false) const;

	const BuildState& m_state;
	const SourceTarget& m_project;
};
}
