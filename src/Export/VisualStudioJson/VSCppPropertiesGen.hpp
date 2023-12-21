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
class BuildState;
struct SourceTarget;

struct VSCppPropertiesGen
{
	explicit VSCppPropertiesGen(const ExportAdapter& inExportAdapter);

	bool saveToFile(const std::string& inFilename);

private:
	std::string getIntellisenseMode(const BuildState& inState, const std::string& inArch) const;

	Json getEnvironments(const BuildState& inState) const;
	Json getGlobalEnvironments(const BuildState& inState) const;
	Json makeEnvironmentVariable(const char* inName, const std::string& inValue) const;

	const SourceTarget* getSignificantTarget(const BuildState& inState) const;

	const ExportAdapter& m_exportAdapter;
	RunConfigurationList m_runConfigs;
};
}
