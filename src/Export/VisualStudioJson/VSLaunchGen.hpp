/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;

struct VSLaunchGen
{
	explicit VSLaunchGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	bool getConfiguration(Json& outConfiguration, const RunConfiguration& runConfig, const BuildState& inState, const IBuildTarget& inTarget) const;
	Json getEnvironment(const IBuildTarget& inTarget) const;

	const ExportAdapter& m_exportAdapter;
	RunConfigurationList m_runConfigs;
};
}
