/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Export/ExportAdapter.hpp"
#include "Utility/Uuid.hpp"
#include "Xml/XmlFile.hpp"

namespace chalet
{
class BuildState;
struct IBuildTarget;
struct SourceTarget;

struct CLionWorkspaceGen
{
	explicit CLionWorkspaceGen(const ExportAdapter& inExportAdapter);

	bool saveToPath(const std::string& inPath);

private:
	bool createCustomTargetsFile(const std::string& inFilename);
	bool createExternalToolsFile(const std::string& inFilename);
	bool createRunConfigurationFile(const std::string& inPath, const ExportRunConfiguration& inRunConfig);
	bool createWorkspaceFile(const std::string& inFilename);
	bool createMiscFile(const std::string& inFilename);
	bool createJsonSchemasFile(const std::string& inFilename);

	std::string getResolvedPath(const std::string& inFile) const;
	std::string getBoolString(const bool inValue) const;
	std::string getNodeIdentifier(const std::string& inName, const ExportRunConfiguration& inRunConfig) const;
	std::string getToolName(const std::string& inLabel, const ExportRunConfiguration& inRunConfig) const;
	std::string getTargetFolderName(const ExportRunConfiguration& inRunConfig) const;

	const ExportAdapter& m_exportAdapter;

	ExportRunConfigurationList m_runConfigs;
	std::map<std::string, std::string> m_toolsMap;

	std::string m_clionNamespaceGuid;
	std::string m_homeDirectory;
	std::string m_ccmdsDirectory;
	std::string m_projectName;
	std::string m_defaultRunTargetName;
	std::string m_chaletPath;
	std::string m_projectId;

	std::string m_settingsFile;
	std::string m_inputFile;

	std::string m_defaultSettingsFile;
	std::string m_defaultInputFile;
	std::string m_yamlInputFile;
};
}
