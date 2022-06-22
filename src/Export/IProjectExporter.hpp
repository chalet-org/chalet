/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IPROJECT_EXPORTER_HPP
#define CHALET_IPROJECT_EXPORTER_HPP

#include "Export/ExportKind.hpp"

namespace chalet
{
struct CentralState;
class BuildState;

struct IProjectExporter;
using ProjectExporter = Unique<IProjectExporter>;

struct IProjectExporter
{
	explicit IProjectExporter(CentralState& inCentralState, const ExportKind inKind);
	CHALET_DISALLOW_COPY_MOVE(IProjectExporter);
	virtual ~IProjectExporter();

	[[nodiscard]] static ProjectExporter make(const ExportKind inKind, CentralState& inCentralState);

	bool generate();

	ExportKind kind() const noexcept;

protected:
	virtual std::string getProjectTypeName() const = 0;
	virtual bool validate(const BuildState& inState) = 0;
	virtual bool generateProjectFiles() = 0;

	bool useExportDirectory(const std::string& inSubDirectory = std::string()) const;

	CentralState& m_centralState;

	std::string m_cwd;
	std::string m_debugConfiguration;

	Dictionary<StringList> m_headerFiles;
	HeapDictionary<BuildState> m_states;

private:
	ExportKind m_kind;
};
}

#endif // CHALET_IPROJECT_EXPORTER_HPP
