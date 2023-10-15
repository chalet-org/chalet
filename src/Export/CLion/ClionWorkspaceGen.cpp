/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CLion/CLionWorkspaceGen.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
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

	auto workspaceFile = fmt::format("{}/workspace.xml", inPath);
	auto customTargetsFile = fmt::format("{}/customTargets.xml", inPath);
	auto externalToolsFile = fmt::format("{}/External Tools.xml", toolsPath);

	auto& debugState = getDebugState();

	m_chaletPath = getResolvedPath(debugState.tools.chalet());

	LOG(workspaceFile);

	if (!createExternalToolsFile(externalToolsFile))
		return false;

	if (!createCustomTargetsFile(customTargetsFile))
		return false;

	return false;
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
			auto name = "Chalet";
			node2.addAttribute("id", Uuid::v5(fmt::format("{}_{}", node2.name(), name), m_clionNamespaceGuid).str());
			node2.addAttribute("name", name);
			node2.addAttribute("defaultType", "TOOL");
			node2.addElement("configuration", [this, &name](XmlElement& node3) {
				node3.addAttribute("id", Uuid::v5(fmt::format("{}_{}", node3.name(), name), m_clionNamespaceGuid).str());
				node3.addAttribute("name", name);
				node3.addElement("build", [](XmlElement& node4) {
					node4.addAttribute("type", "TOOL");
					node4.addElement("tool", [](XmlElement& node5) {
						node5.addAttribute("actionId", "Tool_External Tools_Chalet: Build");
					});
				});
				node3.addElement("clean", [](XmlElement& node4) {
					node4.addAttribute("type", "TOOL");
					node4.addElement("tool", [](XmlElement& node5) {
						node5.addAttribute("actionId", "Tool_External Tools_Chalet: Clean");
					});
				});
			});
		});
	});

	xmlFile.dumpToTerminal();

	if (!xmlFile.save(1))
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

	xmlRoot.setName("toolSet");
	xmlRoot.addAttribute("name", "External Tools");

	std::map<std::string, std::string> tools{
		{ "Build", "build" },
		{ "Clean", "clean" },
	};
	for (auto& it : tools)
	{
		auto& label = it.first;
		auto& cmd = it.second;
		xmlRoot.addElement("tool", [this, &label, &cmd](XmlElement& node) {
			node.addAttribute("name", fmt::format("Chalet: {}", label));
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
					node3.addAttribute("name", "COMMAND");
					node3.addAttribute("value", m_chaletPath);
				});
				node2.addElement("option", [&cmd](XmlElement& node3) {
					node3.addAttribute("name", "PARAMETERS");
					node3.addAttribute("value", cmd);
				});
				node2.addElement("option", [](XmlElement& node3) {
					node3.addAttribute("name", "WORKING_DIRECTORY");
					node3.addAttribute("value", "$ProjectFileDir$");
				});
			});
		});
	}

	xmlFile.dumpToTerminal();

	if (!xmlFile.save(1))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CLionWorkspaceGen::createWorkspaceFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xmlRoot = xmlFile.getRoot();
	UNUSED(xmlRoot);

	if (!xmlFile.save(1))
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
std::string CLionWorkspaceGen::getResolvedPath(const std::string& inFile) const
{
	return Commands::getCanonicalPath(inFile);
}

/*****************************************************************************/
std::string CLionWorkspaceGen::getBoolString(const bool inValue) const
{
	return inValue ? "true" : "false";
}
}
