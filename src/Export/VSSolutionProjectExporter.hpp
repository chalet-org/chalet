/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_VS_SOLUTION_PROJECT_EXPORTER_HPP
#define CHALET_VS_SOLUTION_PROJECT_EXPORTER_HPP

#include "Export/IProjectExporter.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
struct VSSolutionProjectExporter final : public IProjectExporter
{
	explicit VSSolutionProjectExporter(const CommandLineInputs& inInputs);

	std::string getMainProjectOutput(const BuildState& inState);

protected:
	virtual std::string getMainProjectOutput() final;
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	std::string getProjectName(const BuildState& inState) const;

	OrderedDictionary<Uuid> getTargetGuids(const std::string& inProjectTypeGUID, const std::string& inAllBuildName) const;
};
}

#endif // CHALET_VS_SOLUTION_PROJECT_EXPORTER_HPP
