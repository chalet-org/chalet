/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
class BuildState;
struct SourceTarget;

struct CommandAdapterWinResource
{
	explicit CommandAdapterWinResource(const BuildState& inState, const SourceTarget& inProject);

	bool createWindowsApplicationManifest();
	bool createWindowsApplicationIcon();

private:
	const BuildState& m_state;
	const SourceTarget& m_project;
};
}
