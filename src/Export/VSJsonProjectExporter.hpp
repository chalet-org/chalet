/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_JSON_PROJECT_EXPORTER_HPP
#define CHALET_VS_JSON_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct SourceTarget;

struct VSJsonProjectExporter final : public IProjectExporter
{
	explicit VSJsonProjectExporter(const CommandLineInputs& inInputs);

protected:
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	//
};
}

#endif // CHALET_VS_JSON_PROJECT_EXPORTER_HPP
