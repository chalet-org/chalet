/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_EXPORT_KIND_HPP
#define CHALET_EXPORT_KIND_HPP

namespace chalet
{
enum class ExportKind : ushort
{
	None,
	VisualStudioCodeJSON,
	VisualStudioJSON,
	VisualStudioSolution,
	Xcode,
	CodeBlocks,
};
}

#endif // CHALET_EXPORT_KIND_HPP
