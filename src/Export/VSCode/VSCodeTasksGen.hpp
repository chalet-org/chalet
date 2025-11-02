/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Export/VSCode/VSCodeExtensionAwarenessAdapter.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
class BuildState;

struct VSCodeTasksGen
{
	VSCodeTasksGen(const ExportAdapter& inExportAdapter, const VSCodeExtensionAwarenessAdapter& inExtensionAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	bool initialize();

	Json makeRunConfiguration(const ExportRunConfiguration& inRunConfig) const;

	std::string getProblemMatcher() const;

	bool willUseMSVC(const BuildState& inState) const;

	const ExportAdapter& m_exportAdapter;
	const VSCodeExtensionAwarenessAdapter& m_extensionAdapter;

	ExportRunConfigurationList m_runConfigs;

	bool m_usesMsvc = false;
};
}
