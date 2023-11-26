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

struct VSCodeTasksGen
{
	explicit VSCodeTasksGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	bool initialize();

	Json makeRunConfiguration(const RunConfiguration& inRunConfig) const;

	std::string getProblemMatcher() const;

	bool willUseMSVC(const BuildState& inState) const;

	const ExportAdapter& m_exportAdapter;

	RunConfigurationList m_runConfigs;

	bool m_usesMsvc = false;
};
}
