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

protected:
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	bool validateXcodeGenIsInstalled(const std::string& inTypeName);

	std::string m_xcodegen;
};
}

#endif // CHALET_XCODE_PROJECT_EXPORTER_HPP
