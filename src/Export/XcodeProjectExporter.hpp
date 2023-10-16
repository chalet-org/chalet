/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_XCODE_PROJECT_EXPORTER_HPP
#define CHALET_XCODE_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct XcodeProjectExporter final : public IProjectExporter
{
	explicit XcodeProjectExporter(const CommandLineInputs& inInputs);

	std::string getMainProjectOutput(const BuildState& inState);

protected:
	virtual std::string getMainProjectOutput() final;
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;
	virtual bool shouldCleanOnReExport() const final;

private:
	std::string getProjectName(const BuildState& inState) const;
};
}

#endif // CHALET_XCODE_PROJECT_EXPORTER_HPP
