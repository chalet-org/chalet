/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
enum class ExportKind : u16
{
	None,
	VisualStudioCodeJSON,
	VisualStudioJSON,
	VisualStudioSolution,
	VSCodiumJSON,
	Xcode,
	CLion,
	Fleet,
	CodeEdit,
	CodeBlocks,
};
}
