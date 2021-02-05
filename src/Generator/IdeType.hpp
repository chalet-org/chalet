/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IDE_TYPE_HPP
#define CHALET_IDE_TYPE_HPP

namespace chalet
{
enum class IdeType : ushort
{
	None,
	Unknown,
	VisualStudioCode,
	VisualStudio2019,
	XCode,
	CodeBlocks,
};
}

#endif // CHALET_IDE_TYPE_HPP
