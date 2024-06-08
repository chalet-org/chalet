/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct TargetExportAdapter
{
	explicit TargetExportAdapter(const BuildState& inState, const IBuildTarget& inTarget);

	StringList getFiles() const;
	StringList getOutputFiles() const;
	std::string getCommand() const;

private:
	const BuildState& m_state;
	const IBuildTarget& m_target;
};
}
