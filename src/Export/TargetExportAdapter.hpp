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

	bool generateRequiredFiles(const std::string& inLocation) const;

	StringList getFiles() const;
	StringList getOutputFiles() const;
	std::string getCommand() const;
	std::string getRunWorkingDirectory() const;
	std::string getRunWorkingDirectoryWithCurrentWorkingDirectoryAs(const std::string& inAlias) const;

private:
	const BuildState& m_state;
	const IBuildTarget& m_target;
};
}
