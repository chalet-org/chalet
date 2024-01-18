/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Export/ExportKind.hpp"

namespace chalet
{
struct CentralState;
class BuildState;
struct IBuildTarget;
struct CommandLineInputs;

struct IProjectExporter;
using ProjectExporter = Unique<IProjectExporter>;

struct IProjectExporter
{
	explicit IProjectExporter(const CommandLineInputs& inInputs, const ExportKind inKind);
	CHALET_DISALLOW_COPY_MOVE(IProjectExporter);
	virtual ~IProjectExporter();

	[[nodiscard]] static ProjectExporter make(const ExportKind inKind, const CommandLineInputs& inInputs);
	[[nodiscard]] static std::string getProjectBuildFolder(const CommandLineInputs& inInputs);

	bool generate(CentralState& inCentralState, const bool inForBuild = false);

	ExportKind kind() const noexcept;

	std::string getAllBuildTargetName() const;

protected:
	virtual std::string getMainProjectOutput() = 0;
	virtual std::string getProjectTypeName() const = 0;
	virtual bool validate(const BuildState& inState) = 0;
	virtual bool generateProjectFiles() = 0;
	virtual bool openProjectFilesInEditor(const std::string& inProject) = 0;

	virtual bool shouldCleanOnReExport() const;

	const std::string& workingDirectory() const noexcept;

	bool makeStateAndValidate(CentralState& inCentralState, const std::string& configName);
	bool makeExportAdapter();
	bool validateDebugState();

	bool saveSchemasToDirectory(const std::string& inDirectory) const;

	bool useDirectory(const std::string& inDirectory);
	bool useProjectBuildDirectory(const std::string& inSubDirectory);

	void cleanExportDirectory();

	const CommandLineInputs& m_inputs;

	std::string m_directory;
	std::string m_debugConfiguration;

	std::vector<Unique<BuildState>> m_states;

	Unique<ExportAdapter> m_exportAdapter;

private:
	bool generateStatesAndValidate(CentralState& inCentralState);

	ExportKind m_kind;
};
}
