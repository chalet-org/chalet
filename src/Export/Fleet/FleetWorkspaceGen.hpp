/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Libraries/Json.hpp"

namespace chalet
{
class BuildState;

struct FleetWorkspaceGen
{
	explicit FleetWorkspaceGen(const ExportAdapter& inExportAdapter);

	bool saveToPath(const std::string& inPath);

private:
	bool createRunJsonFile(const std::string& inFilename);
	bool createSettingsJsonFile(const std::string& inFilename);

	Json makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const;

	const ExportAdapter& m_exportAdapter;

	ExportRunConfigurationList m_runConfigs;
};
}
