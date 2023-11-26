/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLion/CLionWorkspaceGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "DotEnv/DotEnvFileGenerator.hpp"
#include "Process/Environment.hpp"
#include "Query/QueryController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
/*****************************************************************************/
CLionWorkspaceGen::CLionWorkspaceGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter),
	m_toolsMap({
		{ "Build", "build" },
		{ "Clean", "clean" },
	}),
	m_clionNamespaceGuid("86263C98-993E-44F5-9FE0-D9867378467F")
{
}

/*****************************************************************************/
bool CLionWorkspaceGen::saveToPath(const std::string& inPath)
{
	auto toolsPath = fmt::format("{}/tools", inPath);
	if (!Files::pathExists(toolsPath))
		Files::makeDirectory(toolsPath);

	auto runConfigurationsPath = fmt::format("{}/runConfigurations", inPath);
	if (!Files::pathExists(runConfigurationsPath))
		Files::makeDirectory(runConfigurationsPath);

	auto nameFile = fmt::format("{}/.name", inPath);
	auto workspaceFile = fmt::format("{}/workspace.xml", inPath);
	auto miscFile = fmt::format("{}/misc.xml", inPath);
	auto customTargetsFile = fmt::format("{}/customTargets.xml", inPath);
	auto externalToolsFile = fmt::format("{}/External Tools.xml", toolsPath);
	auto jsonSchemasFile = fmt::format("{}/jsonSchemas.xml", inPath);

	auto& debugState = m_exportAdapter.getDebugState();
	auto& outputDirectory = debugState.paths.outputDirectory();

	m_exportAdapter.createCompileCommandsStub();

	m_homeDirectory = Environment::getUserDirectory();
	m_ccmdsDirectory = fmt::format("$PROJECT_DIR$/{}", outputDirectory);
	m_projectName = String::getPathBaseName(debugState.inputs.workingDirectory());
	m_chaletPath = getResolvedPath(debugState.inputs.appPath());
	m_projectId = Uuid::v5(debugState.workspace.metadata().name(), m_clionNamespaceGuid).str();
	m_settingsFile = debugState.inputs.settingsFile();
	m_inputFile = debugState.inputs.inputFile();

	m_defaultSettingsFile = debugState.inputs.defaultSettingsFile();
	m_defaultInputFile = debugState.inputs.defaultInputFile();
	m_yamlInputFile = debugState.inputs.yamlInputFile();

	auto runTarget = debugState.getFirstValidRunTarget();
	if (runTarget != nullptr)
	{
		m_defaultRunTargetName = runTarget->name();
	}

	Files::createFileWithContents(nameFile, m_projectName);

	m_runConfigs = m_exportAdapter.getFullRunConfigs();

	// Generate CLion files
	if (!createExternalToolsFile(externalToolsFile))
		return false;

	if (!createCustomTargetsFile(customTargetsFile))
		return false;

	if (!createWorkspaceFile(workspaceFile))
		return false;

	if (!createMiscFile(miscFile))
		return false;

	if (!createJsonSchemasFile(jsonSchemasFile))
		return false;

	for (auto& runConfig : m_runConfigs)
	{
		if (!createRunConfigurationFile(runConfigurationsPath, runConfig))
		{
			Diagnostic::error("There was a problem creating the runConfiguration for: {}", runConfig.name);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createCustomTargetsFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlRoot.setName("project");
	xmlRoot.addAttribute("version", "4");
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "CLionExternalBuildManager");

		for (auto& runConfig : m_runConfigs)
		{
			node.addElement("target", [this, &runConfig](XmlElement& node2) {
				node2.addAttribute("id", getNodeIdentifier(node2.name(), runConfig));
				node2.addAttribute("name", m_exportAdapter.getRunConfigLabel(runConfig));
				node2.addAttribute("defaultType", "TOOL");
				node2.addElement("configuration", [this, &runConfig](XmlElement& node3) {
					node3.addAttribute("id", getNodeIdentifier(node3.name(), runConfig));
					node3.addAttribute("name", m_exportAdapter.getRunConfigLabel(runConfig));

					for (auto& it : m_toolsMap)
					{
						auto& label = it.first;
						auto& cmd = it.second;

						node3.addElement(cmd, [this, &label, &runConfig](XmlElement& node4) {
							node4.addAttribute("type", "TOOL");
							node4.addElement("tool", [this, &label, &runConfig](XmlElement& node5) {
								node5.addAttribute("actionId", fmt::format("Tool_External Tools_{}", getToolName(label, runConfig)));
							});
						});
					}
				});
			});
		}
	});

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createExternalToolsFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("toolSet");
	xmlRoot.addAttribute("name", "External Tools");

	for (auto& runConfig : m_runConfigs)
	{
		for (auto& it : m_toolsMap)
		{
			auto& label = it.first;
			auto& cmd = it.second;

			xmlRoot.addElement("tool", [this, &runConfig, &label, &cmd](XmlElement& node) {
				node.addAttribute("name", getToolName(label, runConfig));
				node.addAttribute("description", label);
				node.addAttribute("showInMainMenu", getBoolString(false));
				node.addAttribute("showInEditor", getBoolString(false));
				node.addAttribute("showInProject", getBoolString(false));
				node.addAttribute("showInSearchPopup", getBoolString(false));
				node.addAttribute("disabled", getBoolString(false));
				node.addAttribute("useConsole", getBoolString(true));
				node.addAttribute("showConsoleOnStdOut", getBoolString(false));
				node.addAttribute("showConsoleOnStdErr", getBoolString(true));
				node.addAttribute("synchronizeAfterRun", getBoolString(true));

				node.addElement("exec", [this, &runConfig, &cmd](XmlElement& node2) {
					node2.addElement("option", [this](XmlElement& node3) {
						auto homeDirectory = Environment::getUserDirectory();
						auto chaletPath = m_chaletPath;
						String::replaceAll(chaletPath, m_homeDirectory, "$USER_HOME$");
						node3.addAttribute("name", "COMMAND");
						node3.addAttribute("value", chaletPath);
					});
					node2.addElement("option", [this, &runConfig, &cmd](XmlElement& node3) {
						node3.addAttribute("name", "PARAMETERS");

						auto args = String::join(m_exportAdapter.getRunConfigArguments(runConfig, cmd, false));
						node3.addAttribute("value", args);
					});
					node2.addElement("option", [](XmlElement& node3) {
						node3.addAttribute("name", "WORKING_DIRECTORY");
						node3.addAttribute("value", "$ProjectFileDir$");
					});
				});
			});
		}
	}

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createRunConfigurationFile(const std::string& inPath, const RunConfiguration& inRunConfig)
{
	auto targetName = m_exportAdapter.getRunConfigLabel(inRunConfig);

	auto filename = fmt::format("{}/{}.xml", inPath, targetName);
	XmlFile xmlFile(filename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("component");
	xmlRoot.addAttribute("name", "ProjectRunConfigurationManager");

	xmlRoot.addElement("configuration", [this, &targetName, &inRunConfig](XmlElement& node2) {
		// node2.addAttribute("default", getBoolString(String::equals(m_debugConfiguration, config)));
		node2.addAttribute("name", targetName);
		node2.addAttribute("type", "CLionExternalRunConfiguration");
		node2.addAttribute("factoryName", "Application");
		node2.addAttribute("folderName", getTargetFolderName(inRunConfig));
		node2.addAttribute("REDIRECT_INPUT", getBoolString(false));
		node2.addAttribute("ELEVATE", getBoolString(false));
		node2.addAttribute("USE_EXTERNAL_CONSOLE", getBoolString(false));
		node2.addAttribute("EMULATE_TERMINAL", getBoolString(false));
		node2.addAttribute("PASS_PARENT_ENVS_2", getBoolString(true));
		node2.addAttribute("PROJECT_NAME", m_projectName);
		node2.addAttribute("TARGET_NAME", m_exportAdapter.getRunConfigLabel(inRunConfig));
		node2.addAttribute("RUN_PATH", inRunConfig.outputFile);
		node2.addAttribute("PROGRAM_PARAMS", inRunConfig.args);
		if (!inRunConfig.env.empty())
		{
			node2.addElement("envs", [&inRunConfig](XmlElement& node3) {
				for (auto& it : inRunConfig.env)
				{
					auto& key = it.first;
					auto& value = it.second;

					node3.addElement("env", [&key, &value](XmlElement& node4) {
						node4.addAttribute("name", key);
						node4.addAttribute("value", value);
					});
				}
			});
		}
		node2.addElement("method", [this](XmlElement& node3) {
			node3.addAttribute("v", "2");
			node3.addElement("option", [this](XmlElement& node4) {
				node4.addAttribute("name", "CLION.EXTERNAL.BUILD");
				node4.addAttribute("enabled", getBoolString(true));
			});
		});
	});

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", filename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createWorkspaceFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlRoot.setName("project");
	xmlRoot.addAttribute("version", "4");
	xmlRoot.addElement("component", [](XmlElement& node) {
		node.addAttribute("name", "AutoImportSettings");
		node.addElement("option", [](XmlElement& node2) {
			node2.addAttribute("name", "autoReloadType");
			node2.addAttribute("value", "SELECTIVE");
		});
	});
	xmlRoot.addElement("component", [](XmlElement& node) {
		node.addAttribute("name", "CMakeRunConfigurationManager");
		node.addElement("generated");
	});
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "CMakeSettings");
		node.addElement("configurations", [this](XmlElement& node2) {
			node2.addElement("configurations", [this](XmlElement& node3) {
				node3.addAttribute("PROFILE_NAME", m_exportAdapter.debugConfiguration());
				node3.addAttribute("ENABLED", getBoolString(false));
				node3.addAttribute("CONFIG_NAME", m_exportAdapter.debugConfiguration());
			});
		});
	});

	if (!m_defaultRunTargetName.empty())
	{
		xmlRoot.addElement("component", [this](XmlElement& node) {
			node.addAttribute("name", "CompDBLocalSettings");
			node.addElement("option", [this](XmlElement& node2) {
				node2.addAttribute("name", "availableProjects");
				node2.addElement("map", [this](XmlElement& node3) {
					node3.addElement("entry", [this](XmlElement& node4) {
						node4.addElement("key", [this](XmlElement& node5) {
							node5.addElement("ExternalProjectPojo", [this](XmlElement& node6) {
								node6.addElement("option", [this](XmlElement& node7) {
									node7.addAttribute("name", "name");
									node7.addAttribute("value", m_defaultRunTargetName);
								});
								node6.addElement("option", [this](XmlElement& node7) {
									node7.addAttribute("name", "path");
									node7.addAttribute("value", m_ccmdsDirectory);
								});
							});
						});
						node4.addElement("value", [this](XmlElement& node5) {
							node5.addElement("list", [this](XmlElement& node6) {
								node6.addElement("ExternalProjectPojo", [this](XmlElement& node7) {
									node7.addElement("option", [this](XmlElement& node8) {
										node8.addAttribute("name", "name");
										node8.addAttribute("value", m_defaultRunTargetName);
									});
									node7.addElement("option", [this](XmlElement& node8) {
										node8.addAttribute("name", "path");
										node8.addAttribute("value", m_ccmdsDirectory);
									});
								});
							});
						});
					});
				});
			});
		});
	}

	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "ExternalProjectsData");
		node.addElement("projectState", [this](XmlElement& node2) {
			node2.addAttribute("path", m_ccmdsDirectory);
			node2.addElement("ProjectState");
		});
	});

	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "HighlightingSettingsPerFile");
		node.addElement("setting", [this](XmlElement& node2) {
			node2.addAttribute("file", fmt::format("file://$PROJECT_DIR$/{}", m_settingsFile));
			node2.addAttribute("root0", "FORCE_HIGHLIGHTING");
		});
	});

	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "ProjectId");
		node.addAttribute("id", m_projectId); // TODO: This format maybe - 2WnwGzZ5woZe0F4aLkNaXiztROm
	});

	auto defaultTarget = m_exportAdapter.getDefaultTargetName();
	if (!defaultTarget.empty())
	{
		xmlRoot.addElement("component", [&defaultTarget](XmlElement& node) {
			node.addAttribute("name", "RunManager");
			node.addAttribute("selected", fmt::format("Custom Build Application.{}", defaultTarget));
		});
	}

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createMiscFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlRoot.setName("project");
	xmlRoot.addAttribute("version", "4");
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "CompDBSettings");
		node.addElement("option", [this](XmlElement& node2) {
			node2.addAttribute("name", "linkedExternalProjectsSettings");
			node2.addElement("CompDBProjectSettings", [this](XmlElement& node3) {
				node3.addElement("option", [this](XmlElement& node4) {
					node4.addAttribute("name", "externalProjectPath");
					node4.addAttribute("value", m_ccmdsDirectory);
				});
				node3.addElement("option", [this](XmlElement& node4) {
					node4.addAttribute("name", "modules");
					node4.addElement("set", [this](XmlElement& node5) {
						node5.addElement("option", [this](XmlElement& node6) {
							node6.addAttribute("value", m_ccmdsDirectory);
						});
					});
				});
			});
		});
	});
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "CompDBWorkspace");
		node.addAttribute("PROJECT_DIR", m_ccmdsDirectory);
		node.addElement("contentRoot", [](XmlElement& node2) {
			node2.addAttribute("DIR", "$PROJECT_DIR$");
		});
	});
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "ExternalStorageConfigurationManager");
		node.addAttribute("enabled", getBoolString(true));
	});

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createJsonSchemasFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	chalet_assert(!m_defaultInputFile.empty(), "m_defaultInputFile was empty");
	chalet_assert(!m_yamlInputFile.empty(), "m_yamlInputFile was empty");
	chalet_assert(!m_defaultSettingsFile.empty(), "m_defaultSettingsFile was empty");

	auto& xmlRoot = xmlFile.getRoot();

	xmlRoot.setName("project");
	xmlRoot.addAttribute("version", "4");
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "JsonSchemaMappingsProjectConfiguration");
		node.addElement("state", [this](XmlElement& node2) {
			node2.addElement("map", [this](XmlElement& node3) {
				node3.addElement("entry", [this](XmlElement& node4) {
					node4.addAttribute("key", "chalet.schema");
					node4.addElement("value", [this](XmlElement& node5) {
						node5.addElement("SchemaInfo", [this](XmlElement& node6) {
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "generatedName");
								node7.addAttribute("value", "New Schema");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "name");
								node7.addAttribute("value", "chalet.schema");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "relativePathToSchema");
								node7.addAttribute("value", "$PROJECT_DIR$/.idea/schema/chalet.schema.json");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "schemaVersion");
								node7.addAttribute("value", "JSON Schema version 7");
							});
							node6.addElement("option", [this](XmlElement& node7) {
								node7.addAttribute("name", "patterns");
								node7.addElement("list", [this](XmlElement& node8) {
									node8.addElement("Item", [this](XmlElement& node9) {
										node9.addElement("option", [this](XmlElement& node10) {
											node10.addAttribute("name", "path");
											node10.addAttribute("value", m_defaultInputFile);
										});
									});
									node8.addElement("Item", [this](XmlElement& node9) {
										node9.addElement("option", [this](XmlElement& node10) {
											node10.addAttribute("name", "path");
											node10.addAttribute("value", m_yamlInputFile);
										});
									});
								});
							});
						});
					});
				});
				node3.addElement("entry", [this](XmlElement& node4) {
					node4.addAttribute("key", "chalet.settings.schema");
					node4.addElement("value", [this](XmlElement& node5) {
						node5.addElement("SchemaInfo", [this](XmlElement& node6) {
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "generatedName");
								node7.addAttribute("value", "New Schema");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "name");
								node7.addAttribute("value", "chalet.settings.schema");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "relativePathToSchema");
								node7.addAttribute("value", "$PROJECT_DIR$/.idea/schema/chalet-settings.schema.json");
							});
							node6.addElement("option", [](XmlElement& node7) {
								node7.addAttribute("name", "schemaVersion");
								node7.addAttribute("value", "JSON Schema version 7");
							});
							node6.addElement("option", [this](XmlElement& node7) {
								node7.addAttribute("name", "patterns");
								node7.addElement("list", [this](XmlElement& node8) {
									node8.addElement("Item", [this](XmlElement& node9) {
										node9.addElement("option", [this](XmlElement& node10) {
											node10.addAttribute("name", "path");
											node10.addAttribute("value", m_defaultSettingsFile);
										});
									});
								});
							});
						});
					});
				});
			});
		});
	});

	// xmlFile.dumpToTerminal();

	if (!xmlFile.save(2))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getResolvedPath(const std::string& inFile) const
{
	return Files::getCanonicalPath(inFile);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getBoolString(const bool inValue) const
{
	return inValue ? "true" : "false";
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getNodeIdentifier(const std::string& inName, const RunConfiguration& inRunConfig) const
{
	auto targetName = m_exportAdapter.getRunConfigLabel(inRunConfig);
	return Uuid::v5(fmt::format("{}_{}", inName, targetName), m_clionNamespaceGuid).str();
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getToolName(const std::string& inLabel, const RunConfiguration& inRunConfig) const
{
	auto target = getTargetFolderName(inRunConfig);
	return fmt::format("[{}] [{}] {}", inLabel, target, inRunConfig.name);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getTargetFolderName(const RunConfiguration& inRunConfig) const
{
	auto arch = m_exportAdapter.getLabelArchitecture(inRunConfig);
	return fmt::format("{} {}", arch, inRunConfig.config);
}
}
