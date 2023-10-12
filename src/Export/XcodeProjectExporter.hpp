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

	virtual std::string getMainProjectOutput() final;

protected:
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	std::string getProjectName() const;
};
}

#endif // CHALET_XCODE_PROJECT_EXPORTER_HPP
