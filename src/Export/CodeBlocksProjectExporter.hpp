/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct SourceTarget;
struct CompileToolchainController;

struct CodeBlocksProjectExporter final : public IProjectExporter
{
	explicit CodeBlocksProjectExporter(const CommandLineInputs& inInputs);

	std::string getMainProjectOutput(const BuildState& inState);

protected:
	virtual std::string getMainProjectOutput() final;
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;
	virtual bool shouldCleanOnReExport() const final;
	virtual bool requiresConfigureFiles() const final;
	virtual bool openProjectFilesInEditor(const std::string& inProject) final;

private:
	std::string getProjectName(const BuildState& inState) const;
};
}
