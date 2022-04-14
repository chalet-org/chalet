/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_DISK_IMAGE_CREATOR_HPP
#define CHALET_MACOS_DISK_IMAGE_CREATOR_HPP

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

#endif // CHALET_MACOS_DISK_IMAGE_CREATOR_HPP
