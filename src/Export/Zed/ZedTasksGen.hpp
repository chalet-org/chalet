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

struct ZedTasksGen
{
	explicit ZedTasksGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	bool initialize();

	Json makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const;

	const ExportAdapter& m_exportAdapter;

	ExportRunConfigurationList m_runConfigs;
};
}
