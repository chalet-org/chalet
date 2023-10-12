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

protected:
	virtual std::string getMainProjectOutput() = 0;
	virtual std::string getProjectTypeName() const = 0;
	virtual bool validate(const BuildState& inState) = 0;
	virtual bool generateProjectFiles() = 0;

	const std::string& workingDirectory() const noexcept;

	bool useDirectory(const std::string& inDirectory);
	bool useProjectBuildDirectory(const std::string& inSubDirectory);
	const BuildState* getAnyBuildStateButPreferDebug() const;
	const IBuildTarget* getRunnableTarget(const BuildState& inState) const;

	const CommandLineInputs& m_inputs;

	std::string m_directory;
	std::string m_debugConfiguration;

	Dictionary<StringList> m_headerFiles;
	std::vector<Unique<BuildState>> m_states;
	Dictionary<std::string> m_pathVariables;

private:
	bool generateStatesAndValidate(CentralState& inCentralState);
	void populatePathVariable();

	ExportKind m_kind;
};
}

#endif // CHALET_IPROJECT_EXPORTER_HPP
