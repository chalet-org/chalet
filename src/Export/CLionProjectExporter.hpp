/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_CLION_PROJECT_EXPORTER_HPP
#define CHALET_CLION_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"

namespace chalet
{
struct CLionProjectExporter final : public IProjectExporter
{
	explicit CLionProjectExporter(const CommandLineInputs& inInputs);

protected:
	virtual std::string getMainProjectOutput() final;
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	//
};
}

#endif // CHALET_CLION_PROJECT_EXPORTER_HPP
