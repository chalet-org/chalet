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

struct VSCodeLaunchGen
{
	explicit VSCodeLaunchGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename) const;

private:
	Json getConfiguration(const BuildState& inState) const;
	std::string getName(const BuildState& inState) const;
	std::string getType(const BuildState& inState) const;
	std::string getDebuggerPath(const BuildState& inState) const;
	void setOptions(Json& outJson, const BuildState& inState) const;
	void setPreLaunchTask(Json& outJson) const;
	void setProgramPath(Json& outJson, const BuildState& inState) const;
	void setEnvFilePath(Json& outJson, const BuildState& inState) const;

	bool willUseMSVC(const BuildState& inState) const;
	bool willUseLLDB(const BuildState& inState) const;
	bool willUseGDB(const BuildState& inState) const;

	const ExportAdapter& m_exportAdapter;
};
}
