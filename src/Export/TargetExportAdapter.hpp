/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TARGET_EXPORT_ADAPTER_HPP
#define CHALET_TARGET_EXPORT_ADAPTER_HPP

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct TargetExportAdapter
{
	explicit TargetExportAdapter(const BuildState& inState, const IBuildTarget& inTarget);

	StringList getFiles() const;
	std::string getCommand() const;

private:
	const BuildState& m_state;
	const IBuildTarget& m_target;
};
}

#endif // CHALET_TARGET_EXPORT_ADAPTER_HPP
