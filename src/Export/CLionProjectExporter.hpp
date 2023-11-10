/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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
