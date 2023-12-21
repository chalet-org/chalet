/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Platform/Arch.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CentralState;
class BuildState;
struct IBuildTarget;

struct VSLaunchGen
{
	explicit VSLaunchGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	Json getConfiguration(const RunConfiguration& runConfig, const CentralState& inCentralState, const IBuildTarget& inTarget) const;
	Json getEnvironment(const IBuildTarget& inTarget) const;

	const ExportAdapter& m_exportAdapter;
	RunConfigurationList m_runConfigs;
};
}
