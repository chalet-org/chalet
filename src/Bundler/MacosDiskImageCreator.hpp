/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct MacosDiskImageTarget;
class BuildState;

struct MacosDiskImageCreator
{
	explicit MacosDiskImageCreator(const BuildState& inState);

	bool make(const MacosDiskImageTarget& inDiskImage);

private:
	bool signDmgImage(const std::string& inPath) const;
	std::string getDmgApplescript(const MacosDiskImageTarget& inDiskImage) const;

	const BuildState& m_state;

	std::string m_diskName;

	Dictionary<std::string> m_includedPaths;
};
}
