/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocks/CodeBlocksWorkspaceGen.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksWorkspaceGen::CodeBlocksWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, const std::string& inAllBuildName) :
	m_states(inStates),
	m_debugConfiguration(inDebugConfig),
	m_allBuildName(inAllBuildName)
{
}

/*****************************************************************************/
bool CodeBlocksWorkspaceGen::saveToFile(const std::string& inFilename)
{
	if (!createWorkspaceFile(inFilename))
		return false;

	if (!createWorkspaceLayoutFile(fmt::format("{}.layout", inFilename)))
		return false;

	return true;
}

/*****************************************************************************/
bool CodeBlocksWorkspaceGen::createWorkspaceFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();

	auto& debugState = getDebugState();

	std::map<std::string, StringList> dependsList;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			const auto& name = target->name();
			if (dependsList.find(name) == dependsList.end())
				dependsList.emplace(name, StringList{});

			state->getTargetDependencies(dependsList[name], name, false);
		}
	}

	{
		StringList allDepends;
		for (auto& [target, _] : dependsList)
		{
			allDepends.push_back(target);
		}
		dependsList.emplace(m_allBuildName, std::move(allDepends));
	}

	xmlRoot.setName("CodeBlocks_workspace_file");
	xmlRoot.addElement("Workspace", [this, &debugState, &dependsList](XmlElement& node) {
		const auto& workspaceName = debugState.workspace.metadata().name();
		node.addAttribute("title", workspaceName);

		auto runTarget = debugState.getFirstValidRunTarget();
		for (auto& it : dependsList)
		{
			auto& name = it.first;
			auto& depends = it.second;
			node.addElement("Project", [this, &name, &depends, &runTarget](XmlElement& node2) {
				node2.addAttribute("filename", getRelativeProjectPath(name));

				if (runTarget != nullptr && String::equals(runTarget->name(), name))
					node2.addAttribute("active", "1");

				for (auto& depend : depends)
				{
					node2.addElement("Depends", [this, &depend](XmlElement& node3) {
						node3.addAttribute("filename", getRelativeProjectPath(depend));
					});
				}
			});
		}
	});

	xml.setStandalone(true);
	if (!xmlFile.save(1))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CodeBlocksWorkspaceGen::createWorkspaceLayoutFile(const std::string& inFilename)
{
	XmlFile xmlFile(inFilename);

	auto& xml = xmlFile.xml;
	auto& xmlRoot = xml.root();

	auto& debugState = getDebugState();
	auto runTarget = debugState.getFirstValidRunTarget();

	xmlRoot.setName("CodeBlocks_workspace_layout_file");
	if (runTarget != nullptr)
	{
		xmlRoot.addElement("ActiveProject", [this, &runTarget](XmlElement& node) {
			node.addAttribute("path", getRelativeProjectPath(runTarget->name()));
		});
	}
	xmlRoot.addElement("PreferredTarget", [this](XmlElement& node) {
		node.addAttribute("name", m_debugConfiguration);
	});

	xml.setStandalone(true);
	if (!xmlFile.save(1))
	{
		Diagnostic::error("There was a problem saving: {}", inFilename);
		return false;
	}

	return true;
}

/*****************************************************************************/
BuildState& CodeBlocksWorkspaceGen::getDebugState() const
{
	for (auto& state : m_states)
	{
		if (String::equals(m_debugConfiguration, state->configuration.name()))
			return *state;
	}

	return *m_states.front();
}

/*****************************************************************************/
std::string CodeBlocksWorkspaceGen::getRelativeProjectPath(const std::string& inName) const
{
	return fmt::format("cbp/{}.cbp", inName);
}

}
