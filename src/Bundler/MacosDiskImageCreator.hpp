/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MACOS_DISK_IMAGE_CREATOR_HPP
#define CHALET_MACOS_DISK_IMAGE_CREATOR_HPP

namespace chalet
{
struct CommandLineInputs;
struct MacosDiskImageTarget;
struct StatePrototype;

struct MacosDiskImageCreator
{
	explicit MacosDiskImageCreator(const CommandLineInputs& inInputs, const StatePrototype& inPrototype);

	bool make(const MacosDiskImageTarget& inDiskImage);

private:
	bool signDmgImage(const std::string& inPath) const;
	std::string getDmgApplescript(const MacosDiskImageTarget& inDiskImage) const;

	const CommandLineInputs& m_inputs;
	const StatePrototype& m_prototype;

	std::string m_diskName;

	Dictionary<std::string> m_includedPaths;
};
}

#endif // CHALET_MACOS_DISK_IMAGE_CREATOR_HPP
