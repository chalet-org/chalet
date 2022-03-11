/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/IProjectExporter.hpp"

#include "Export/CodeBlocksProjectExporter.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
IProjectExporter::IProjectExporter(const BuildState& inState, const ExportKind inKind) :
	m_state(inState),
	m_kind(inKind)
{
}

/*****************************************************************************/
[[nodiscard]] ProjectExporter IProjectExporter::make(const ExportKind inKind, const BuildState& inState)
{
	switch (inKind)
	{
		case ExportKind::CodeBlocks:
			return std::make_unique<CodeBlocksProjectExporter>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ProjectExporter requested: ", static_cast<int>(inKind));
	return nullptr;
}

/*****************************************************************************/
ExportKind IProjectExporter::kind() const noexcept
{
	return m_kind;
}

}
