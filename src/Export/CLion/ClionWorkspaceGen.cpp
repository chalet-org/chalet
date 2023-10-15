/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLion/CLionWorkspaceGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Core/QueryController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
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

	m_arches = getArchitectures();

	if (!createExternalToolsFile(externalToolsFile))
		return false;

	if (!createCustomTargetsFile(customTargetsFile))
		return false;

	if (!createWorkspaceFile(workspaceFile))
		return false;

	for (auto& arch : m_arches)
	{
		for (auto& state : m_states)
		{
			for (auto& target : state->targets)
			{
				if (target->isSources())
				{
					auto& sourceTarget = static_cast<const SourceTarget&>(*target);
					if (!sourceTarget.isExecutable())
						continue;

					if (!createRunConfigurationFile(runConfigurationsPath, *state, sourceTarget, arch))
					{
						Diagnostic::error("There was a problem creating the runConfiguration for: {}", sourceTarget.name());
						return false;
					}
				}
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

		for (auto& arch : m_arches)
		{
			for (auto& state : m_states)
			{
				auto& config = state->configuration.name();
				node.addElement("target", [this, &arch, &config](XmlElement& node2) {
					node2.addAttribute("id", Uuid::v5(fmt::format("{}_{}_{}", node2.name(), arch, config), m_clionNamespaceGuid).str());
					node2.addAttribute("name", getTargetFolderName(arch, config));
					node2.addAttribute("defaultType", "TOOL");
					node2.addElement("configuration", [this, &arch, &config](XmlElement& node3) {
						node3.addAttribute("id", Uuid::v5(fmt::format("{}_{}_{}", node3.name(), arch, config), m_clionNamespaceGuid).str());
						node3.addAttribute("name", getTargetFolderName(arch, config));

						for (auto& it : m_toolsMap)
						{
							auto& label = it.first;
							auto& cmd = it.second;

							node3.addElement(cmd, [this, &label, &arch, &config](XmlElement& node4) {
								node4.addAttribute("type", "TOOL");
								node4.addElement("tool", [this, &label, &arch, &config](XmlElement& node5) {
									node5.addAttribute("actionId", fmt::format("Tool_External Tools_{}", getToolName(label, arch, config)));
								});
							});
						}
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
bool CLionWorkspaceGen::createExternalToolsFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("toolSet");
	xmlRoot.addAttribute("name", "External Tools");

	for (auto& arch : m_arches)
	{
		for (auto& state : m_states)
		{
			auto& config = state->configuration.name();
			auto& toolchain = state->inputs.toolchainPreferenceName();
			for (auto& it : m_toolsMap)
			{
				auto& label = it.first;
				auto& cmd = it.second;

				xmlRoot.addElement("tool", [this, &arch, &config, &toolchain, &label, &cmd](XmlElement& node) {
					node.addAttribute("name", getToolName(label, arch, config));
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

					node.addElement("exec", [this, &arch, &config, &toolchain, &cmd](XmlElement& node2) {
						node2.addElement("option", [this](XmlElement& node3) {
							auto homeDirectory = Environment::getUserDirectory();
							auto chaletPath = m_chaletPath;
							String::replaceAll(chaletPath, m_homeDirectory, "$USER_HOME$");
							node3.addAttribute("name", "COMMAND");
							node3.addAttribute("value", chaletPath);
						});
						node2.addElement("option", [this, &arch, &config, &toolchain, &cmd](XmlElement& node3) {
							node3.addAttribute("name", "PARAMETERS");
							node3.addAttribute("value", fmt::format("-c {} -a {} -t {} {}", config, arch, toolchain, cmd));
						});
						node2.addElement("option", [](XmlElement& node3) {
							node3.addAttribute("name", "WORKING_DIRECTORY");
							node3.addAttribute("value", "$ProjectFileDir$");
						});
					});
				});
			}
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
bool CLionWorkspaceGen::createRunConfigurationFile(const std::string& inPath, const BuildState& inState, const SourceTarget& inTarget, const std::string& inArch)
{
	auto& config = inState.configuration.name();

	auto targetName = getTargetName(inTarget.name(), config, inArch);
	auto filename = fmt::format("{}/{}.xml", inPath, targetName);
	XmlFile xmlFile(filename);

	auto& xmlRoot = xmlFile.getRoot();

	xmlFile.xml.setUseHeader(false);
	xmlRoot.setName("component");
	xmlRoot.addAttribute("name", "ProjectRunConfigurationManager");

	const auto& thisArch = inState.info.targetArchitectureString();
	auto buildDir = inState.paths.buildOutputDir();
	String::replaceAll(buildDir, thisArch, inArch);
	auto outputFile = fmt::format("{}/{}", buildDir, inTarget.outputFile());

	xmlRoot.addElement("configuration", [this, &targetName, &inArch, &config, &outputFile](XmlElement& node2) {
		// node2.addAttribute("default", getBoolString(String::equals(m_debugConfiguration, config)));
		node2.addAttribute("name", targetName);
		node2.addAttribute("type", "CLionExternalRunConfiguration");
		node2.addAttribute("factoryName", "Application");
		node2.addAttribute("folderName", getTargetFolderName(inArch, config));
		node2.addAttribute("REDIRECT_INPUT", getBoolString(false));
		node2.addAttribute("ELEVATE", getBoolString(false));
		node2.addAttribute("USE_EXTERNAL_CONSOLE", getBoolString(false));
		node2.addAttribute("EMULATE_TERMINAL", getBoolString(false));
		node2.addAttribute("PASS_PARENT_ENVS_2", getBoolString(true));
		node2.addAttribute("PROJECT_NAME", m_projectName);
		node2.addAttribute("TARGET_NAME", getTargetFolderName(inArch, config));
		node2.addAttribute("RUN_PATH", outputFile);
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
	xmlRoot.addElement("component", [this](XmlElement& node) {
		node.addAttribute("name", "ProjectId");
		node.addAttribute("id", m_projectId); // TODO: This format maybe - 2WnwGzZ5woZe0F4aLkNaXiztROm
	});

	auto defaultTarget = getDefaultTargetName();
	if (!defaultTarget.empty())
	{
		xmlRoot.addElement("component", [this, &defaultTarget](XmlElement& node) {
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
StringList CLionWorkspaceGen::getArchitectures() const
{
	auto& debugState = getDebugState();
	auto& toolchain = debugState.inputs.toolchainPreferenceName();

	StringList excludes{ "auto" };
#if defined(CHALET_WIN32)
	if (debugState.environment->isMsvc())
	{
		excludes.emplace_back("x64_x64");
		excludes.emplace_back("x64_x86");
		excludes.emplace_back("x64_arm64");
		excludes.emplace_back("x64_arm");
		excludes.emplace_back("x86_x86");
		excludes.emplace_back("x86_x64");
		excludes.emplace_back("x86_arm64");
		excludes.emplace_back("x86_arm");
		excludes.emplace_back("x64");
		excludes.emplace_back("x86");
	}
#endif

	StringList ret;
	QueryController query(debugState.getCentralState());
	auto arches = query.getArchitectures(toolchain);
	for (auto&& arch : arches)
	{
		if (String::equals(excludes, arch))
			continue;

		ret.emplace_back(std::move(arch));
	}

	if (ret.empty())
	{
		ret.emplace_back(debugState.info.hostArchitectureString());
	}

	return ret;
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
std::string CLionWorkspaceGen::getTargetName(const std::string& inName, const std::string& inConfig, const std::string& inArch) const
{
	auto target = getTargetFolderName(inArch, inConfig);
	return fmt::format("{} - {}", inName, target);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getToolName(const std::string& inLabel, const std::string& inArch, const std::string& inConfig) const
{
	auto target = getTargetFolderName(inArch, inConfig);
	return fmt::format("Chalet: {} {}", inLabel, target);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getTargetFolderName(const std::string& inArch, const std::string& inConfig) const
{
	auto arch = inArch;
	String::replaceAll(arch, "x86_64", "x64");
	String::replaceAll(arch, "i686", "x86");

	return fmt::format("{} [{}]", arch, inConfig);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getDefaultTargetName() const
{
	auto& debugState = getDebugState();
	auto target = debugState.getFirstValidRunTarget();
	if (target == nullptr)
		return std::string();

	auto& config = debugState.configuration.name();
	auto& arch = debugState.info.hostArchitectureString();

	return getTargetName(target->name(), config, arch);
}
}
