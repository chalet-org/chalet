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
struct SourceTarget;

struct VSSolutionProjectExporter final : public IProjectExporter
{
	explicit VSSolutionProjectExporter(CentralState& inCentralState);

protected:
	virtual std::string getProjectTypeName() const final;
	virtual bool validate(const BuildState& inState) final;
	virtual bool generateProjectFiles() final;

private:
	OrderedDictionary<Uuid> getTargetGuids(const std::string& inProjectTypeGUID) const;
};
}

#endif // CHALET_VS_SOLUTION_PROJECT_EXPORTER_HPP
