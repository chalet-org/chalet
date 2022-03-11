/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IPROJECT_EXPORTER_HPP
#define CHALET_IPROJECT_EXPORTER_HPP

#include "Export/ExportKind.hpp"

namespace chalet
{
class BuildState;

struct IProjectExporter;
using ProjectExporter = Unique<IProjectExporter>;

struct IProjectExporter
{
	explicit IProjectExporter(const BuildState& inState, const ExportKind inKind);
	CHALET_DEFAULT_COPY_MOVE(IProjectExporter);
	virtual ~IProjectExporter() = default;

	[[nodiscard]] static ProjectExporter make(const ExportKind inKind, const BuildState& inState);

	virtual bool validate() = 0;
	virtual bool generate() = 0;

	ExportKind kind() const noexcept;

protected:
	const BuildState& m_state;

private:
	ExportKind m_kind;
};
}

#endif // CHALET_IPROJECT_EXPORTER_HPP
