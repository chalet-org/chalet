/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMMAND_ADAPTER_WIN_RESOURCE_HPP
#define CHALET_COMMAND_ADAPTER_WIN_RESOURCE_HPP

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

#endif // CHALET_COMMAND_ADAPTER_WIN_RESOURCE_HPP
