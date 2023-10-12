/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VSCODE_PROJECT_EXPORTER_HPP
#define CHALET_VSCODE_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct VSCodeProjectExporter final : public IProjectExporter
{
	explicit VSCodeProjectExporter(const CommandLineInputs& inInputs);

protected:
	virtual std::string getMainProjectOutput() final;
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	//
};
}

#endif // CHALET_VSCODE_PROJECT_EXPORTER_HPP
