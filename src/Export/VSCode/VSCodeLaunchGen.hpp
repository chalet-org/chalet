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
struct IBuildTarget;

struct VSCodeLaunchGen
{
	VSCodeLaunchGen(const ExportAdapter& inExportAdapter, const VSCodeExtensionAwarenessAdapter& inExtensionAdapter);

	bool saveToFile(const std::string& inFilename) const;

private:
	bool getConfiguration(Json& outConfiguration, const BuildState& inState) const;
	std::string getCppToolsDebugName(const BuildState& inState) const;
	std::string getCppToolsDebugType(const BuildState& inState) const;
	std::string getDebuggerPath(const BuildState& inState) const;
	bool setCodeLLDBOptions(Json& outJson, const BuildState& inState) const;
	bool setCppToolsDebugOptions(Json& outJson, const BuildState& inState) const;
	void setPreLaunchTask(Json& outJson) const;
	std::string getWorkingDirectory(const BuildState& inState) const;
	bool setProgramPathAndArguments(Json& outJson, const BuildState& inState) const;
	std::string getEnvFilePath(const BuildState& inState) const;

	bool willUseMSVC(const BuildState& inState) const;
	bool willUseLLDB(const BuildState& inState) const;
	bool willUseGDB(const BuildState& inState) const;

	const ExportAdapter& m_exportAdapter;
	const VSCodeExtensionAwarenessAdapter& m_extensionAdapter;
};
}
