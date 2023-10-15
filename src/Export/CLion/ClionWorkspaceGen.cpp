/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLion/CLionWorkspaceGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"
#include "Utility/Uuid.hpp"

namespace chalet
{
/*****************************************************************************/
CLionWorkspaceGen::CLionWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig) :
	m_states(inStates),
	m_debugConfiguration(inDebugConfig),
	m_clionNamespaceGuid("86263C98-993E-44F5-9FE0-D9867378467F")
{
}

/*****************************************************************************/
bool CLionWorkspaceGen::saveToPath(const std::string& inPath)
{
	UNUSED(inPath);

	auto toolsPath = fmt::format("{}/tools", inPath);
	if (!Commands::pathExists(toolsPath))
		Commands::makeDirectory(toolsPath);

	auto runConfigurationsPath = fmt::format("{}/runConfigurations", inPath);
	if (!Commands::pathExists(runConfigurationsPath))
		Commands::makeDirectory(runConfigurationsPath);

	auto nameFile = fmt::format("{}/.name", inPath);
	auto workspaceFile = fmt::format("{}/workspace.xml", inPath);
	auto customTargetsFile = fmt::format("{}/customTargets.xml", inPath);
	auto externalToolsFile = fmt::format("{}/External Tools.xml", toolsPath);

	auto& debugState = getDebugState();

	m_homeDirectory = Environment::getUserDirectory();
	m_projectName = String::getPathBaseName(debugState.inputs.workingDirectory());
	m_chaletPath = getResolvedPath(debugState.tools.chalet());
	m_projectId = Uuid::v5(debugState.workspace.metadata().name(), m_clionNamespaceGuid).str();

	Commands::createFileWithContents(nameFile, m_projectName);

	if (!createExternalToolsFile(externalToolsFile))
		return false;

	if (!createCustomTargetsFile(customTargetsFile))
		return false;

	if (!createWorkspaceFile(workspaceFile))
		return false;

	for (auto& target : debugState.targets)
	{
		if (target->isSources())
		{
			auto& sourceTarget = static_cast<const SourceTarget&>(*target);
			if (!sourceTarget.isExecutable())
				continue;

			if (!createRunConfigurationFile(runConfigurationsPath, sourceTarget))
			{
				Diagnostic::error("There was a problem creating the runConfiguration for: {}", sourceTarget.name());
				return false;
			}
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
		node.addElement("target", [this](XmlElement& node2) {
			auto name = getTargetName();
			node2.addAttribute("id", Uuid::v5(fmt::format("{}_{}", node2.name(), name), m_clionNamespaceGuid).str());
			node2.addAttribute("name", name);
			node2.addAttribute("defaultType", "TOOL");
			node2.addElement("configuration", [this, &name](XmlElement& node3) {
				node3.addAttribute("id", Uuid::v5(fmt::format("{}_{}", node3.name(), name), m_clionNamespaceGuid).str());
				node3.addAttribute("name", name);

				auto toolsMap = getToolsMap();

				for (auto& it : toolsMap)
				{
					auto& label = it.first;
					auto& cmd = it.second;

					node3.addElement(cmd, [&label](XmlElement& node4) {
						node4.addAttribute("type", "TOOL");
						node4.addElement("tool", [&label](XmlElement& node5) {
							node5.addAttribute("actionId", fmt::format("Tool_External Tools_Chalet: {}", label));
						});
					});
				}
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
bool CLionWorkspaceGen::createExternalToolsFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("toolSet");
	xmlRoot.addAttribute("name", "External Tools");

	auto toolsMap = getToolsMap();
	for (auto& it : toolsMap)
	{
		auto& label = it.first;
		auto& cmd = it.second;

		xmlRoot.addElement("tool", [this, &label, &cmd](XmlElement& node) {
			auto name = getTargetName();
			node.addAttribute("name", fmt::format("{}: {}", name, label));
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

			node.addElement("exec", [this, &cmd](XmlElement& node2) {
				node2.addElement("option", [this](XmlElement& node3) {
					auto homeDirectory = Environment::getUserDirectory();
					auto chaletPath = m_chaletPath;
					String::replaceAll(chaletPath, m_homeDirectory, "$USER_HOME$");
					node3.addAttribute("name", "COMMAND");
					node3.addAttribute("value", chaletPath);
				});
				node2.addElement("option", [this, &cmd](XmlElement& node3) {
					node3.addAttribute("name", "PARAMETERS");
					node3.addAttribute("value", fmt::format("-c {} {}", m_debugConfiguration, cmd));
				});
				node2.addElement("option", [](XmlElement& node3) {
					node3.addAttribute("name", "WORKING_DIRECTORY");
					node3.addAttribute("value", "$ProjectFileDir$");
				});
			});
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
bool CLionWorkspaceGen::createRunConfigurationFile(const std::string& inPath, const SourceTarget& inTarget)
{
	auto filename = fmt::format("{}/{}.xml", inPath, inTarget.name());
	XmlFile xmlFile(filename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("component");
	xmlRoot.addAttribute("name", "ProjectRunConfigurationManager");

	auto& debugState = getDebugState();

	auto outputFile = debugState.paths.getTargetFilename(inTarget);
	xmlRoot.addElement("configuration", [this, &inTarget, &outputFile](XmlElement& node2) {
		auto& name = inTarget.name();
		node2.addAttribute("default", getBoolString(false));
		node2.addAttribute("name", name);
		node2.addAttribute("type", "CLionExternalRunConfiguration");
		node2.addAttribute("factoryName", "Application");
		node2.addAttribute("REDIRECT_INPUT", getBoolString(false));
		node2.addAttribute("ELEVATE", getBoolString(false));
		node2.addAttribute("USE_EXTERNAL_CONSOLE", getBoolString(false));
		node2.addAttribute("EMULATE_TERMINAL", getBoolString(false));
		node2.addAttribute("PASS_PARENT_ENVS_2", getBoolString(true));
		node2.addAttribute("PROJECT_NAME", m_projectName);
		node2.addAttribute("TARGET_NAME", getTargetName());
		node2.addAttribute("RUN_PATH", outputFile);
		node2.addElement("envs", [&name](XmlElement& node3) {
			node3.addElement("env", [&name](XmlElement& node4) {
				node4.addAttribute("name", "CLION_CHALET_TARGET");
				node4.addAttribute("value", name);
			});
		});
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

	/*
	<?xml version="1.0" encoding="UTF-8"?>
	<project version="4">
		<component name="CMakeRunConfigurationManager">
			<generated />
		</component>
		<component name="CMakeSettings">
			<configurations>
				<configuration PROFILE_NAME="Debug" ENABLED="false" CONFIG_NAME="Debug" />
			</configurations>
		</component>

		<component name="ChangeListManager">
			<list default="true" id="a37c1c0a-61fc-4c5d-bdd8-14122cbefe95" name="Changes" comment="" />
			<option name="SHOW_DIALOG" value="false" />
			<option name="HIGHLIGHT_CONFLICTS" value="true" />
			<option name="HIGHLIGHT_NON_ACTIVE_CHANGELIST" value="false" />
			<option name="LAST_RESOLUTION" value="IGNORE" />
		</component>
		<component name="ClangdSettings">
			<option name="formatViaClangd" value="false" />
		</component>
		<component name="ProjectColorInfo"><![CDATA[{
		"customColor": "",
		"associatedIndex": 6
		}]]></component>
		<component name="ProjectId" id="2WnwGzZ5woZe0F4aLkNaXiztROm" />
		<component name="ProjectViewState">
			<option name="hideEmptyMiddlePackages" value="true" />
			<option name="showLibraryContents" value="true" />
		</component>
		<component name="PropertiesComponent"><![CDATA[{
		"keyToString": {
			"RunOnceActivity.OpenProjectViewOnStart": "true",
			"RunOnceActivity.ShowReadmeOnStart": "true",
			"RunOnceActivity.cidr.known.project.marker": "true",
			"WebServerToolWindowFactoryState": "false",
			"cf.first.check.clang-format": "false",
			"cidr.known.project.marker": "true",
			"ignore.virus.scanning.warn.message": "true",
			"node.js.detected.package.eslint": "true",
			"node.js.detected.package.tslint": "true",
			"node.js.selected.package.eslint": "(autodetect)",
			"node.js.selected.package.tslint": "(autodetect)",
			"settings.editor.selected.configurable": "CLionExternalConfigurable",
			"settings.editor.splitter.proportion": "0.26069248",
			"vue.rearranger.settings.migration": "true"
		}
		}]]></component>
		<component name="RunManager">
			<configuration name="new-project" type="CLionExternalRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true" PROJECT_NAME="new-project" TARGET_NAME="Chalet" CONFIG_NAME="Chalet" RUN_PATH="build/x86_64-pc-windows-msvc_Release/new-project.exe">
			<envs>
				<env name="CLION_CHALET_TARGET" value="new-project" />
			</envs>
			<method v="2">
				<option name="CLION.EXTERNAL.BUILD" enabled="true" />
			</method>
			</configuration>
			<configuration default="true" type="CMakeRunConfiguration" factoryName="Application" REDIRECT_INPUT="false" ELEVATE="false" USE_EXTERNAL_CONSOLE="false" EMULATE_TERMINAL="false" PASS_PARENT_ENVS_2="true">
			<method v="2">
				<option name="com.jetbrains.cidr.execution.CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask" enabled="true" />
			</method>
			</configuration>
		</component>
		<component name="SpellCheckerSettings" RuntimeDictionaries="0" Folders="0" CustomDictionaries="0" DefaultDictionary="application-level" UseSingleDictionary="true" transferred="true" />
		<component name="TaskManager">
			<task active="true" id="Default" summary="Default task">
			<changelist id="a37c1c0a-61fc-4c5d-bdd8-14122cbefe95" name="Changes" comment="" />
			<created>1697381560836</created>
			<option name="number" value="Default" />
			<option name="presentableId" value="Default" />
			<updated>1697381560836</updated>
			<workItem from="1697381562483" duration="1984000" />
			</task>
			<servers />
		</component>
		<component name="TypeScriptGeneratedFilesManager">
			<option name="version" value="3" />
		</component>
	</project>
	*/

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
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "ProjectId");
		node.addAttribute("id", m_projectId); // TODO: This format maybe - 2WnwGzZ5woZe0F4aLkNaXiztROm
	});
	/*
		<component name="RunManager">
			<configuration
				default="true"
				type="CMakeRunConfiguration"
				factoryName="Application"
				REDIRECT_INPUT="false"
				ELEVATE="false"
				USE_EXTERNAL_CONSOLE="false"
				EMULATE_TERMINAL="false"
				PASS_PARENT_ENVS_2="true"
			>
				<method v="2">
					<option name="com.jetbrains.cidr.execution.CidrBuildBeforeRunTaskProvider$BuildBeforeRunTask" enabled="true" />
				</method>
			</configuration>

			<configuration
				name="new-project"
				type="CLionExternalRunConfiguration"
				factoryName="Application"
				REDIRECT_INPUT="false"
				ELEVATE="false"
				USE_EXTERNAL_CONSOLE="false"
				EMULATE_TERMINAL="false"
				PASS_PARENT_ENVS_2="true"
				PROJECT_NAME="new-project"
				TARGET_NAME="Chalet"
				CONFIG_NAME="Chalet"
				RUN_PATH="build/x86_64-pc-windows-msvc_Release/new-project.exe"
			>
				<envs>
					<env name="CLION_CHALET_TARGET" value="new-project" />
				</envs>
				<method v="2">
					<option name="CLION.EXTERNAL.BUILD" enabled="true" />
				</method>
			</configuration>
		</component>
	*/
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "RunManager");

		// For each exectuable target
		auto& debugState = getDebugState();
		for (auto& target : debugState.targets)
		{
			if (target->isSources())
			{
				auto& sourceTarget = static_cast<const SourceTarget&>(*target);
				if (!sourceTarget.isExecutable())
					continue;

				auto outputFile = debugState.paths.getTargetFilename(sourceTarget);

				node.addElement("configuration", [this, &sourceTarget, &outputFile](XmlElement& node2) {
					auto& name = sourceTarget.name();
					node2.addAttribute("name", name);
					node2.addAttribute("type", "CLionExternalRunConfiguration");
					node2.addAttribute("factoryName", "Application");
					node2.addAttribute("REDIRECT_INPUT", getBoolString(false));
					node2.addAttribute("ELEVATE", getBoolString(false));
					node2.addAttribute("USE_EXTERNAL_CONSOLE", getBoolString(false));
					node2.addAttribute("EMULATE_TERMINAL", getBoolString(false));
					node2.addAttribute("PASS_PARENT_ENVS_2", getBoolString(true));
					node2.addAttribute("PROJECT_NAME", name);
					node2.addAttribute("TARGET_NAME", getTargetName());
					node2.addAttribute("CONFIG_NAME", name);
					node2.addAttribute("RUN_PATH", outputFile);
					node2.addElement("envs", [&name](XmlElement& node3) {
						node3.addElement("env", [&name](XmlElement& node4) {
							node4.addAttribute("name", "CLION_CHALET_TARGET");
							node4.addAttribute("value", name);
						});
					});
					node2.addElement("method", [this](XmlElement& node3) {
						node3.addAttribute("v", "2");
						node3.addElement("option", [this](XmlElement& node4) {
							node4.addAttribute("name", "CLION.EXTERNAL.BUILD");
							node4.addAttribute("enabled", getBoolString(true));
						});
					});
				});
			}
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
BuildState& CLionWorkspaceGen::getDebugState() const
{
	for (auto& state : m_states)
	{
		if (String::equals(m_debugConfiguration, state->configuration.name()))
			return *state;
	}

	return *m_states.front();
}

/*****************************************************************************/
std::map<std::string, std::string> CLionWorkspaceGen::getToolsMap() const
{
	return {
		{ "Build", "build" },
		{ "Clean", "clean" },
	};
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getResolvedPath(const std::string& inFile) const
{
	return Commands::getCanonicalPath(inFile);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getBoolString(const bool inValue) const
{
	return inValue ? "true" : "false";
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getTargetName() const
{
	return "Chalet";
}
}
