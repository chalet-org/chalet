/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_TARGET_ADAPTER_VCXPROJ_HPP
#define CHALET_TARGET_ADAPTER_VCXPROJ_HPP

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct TargetAdapterVCXProj
{
	explicit TargetAdapterVCXProj(const BuildState& inState, const IBuildTarget& inTarget, const std::string& inCwd);

	StringList getFiles() const;
	std::string getCommand() const;

private:
	const BuildState& m_state;
	const IBuildTarget& m_target;
	const std::string& m_cwd;
};
}

#endif // CHALET_TARGET_ADAPTER_VCXPROJ_HPP
