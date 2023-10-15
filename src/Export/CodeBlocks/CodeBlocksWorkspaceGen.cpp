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
		StringList lastDependencies;
		bool lastDependencyWasSource = false;
		for (auto& target : state->targets)
		{
			const auto& name = target->name();
			if (dependsList.find(name) == dependsList.end())
				dependsList.emplace(name, StringList{});

			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);
				if (!lastDependencyWasSource && !lastDependencies.empty())
				{
					for (auto& dep : lastDependencies)
						List::addIfDoesNotExist(dependsList[name], dep);
				}

				auto projectDepends = List::combine(project.projectStaticLinks(), project.projectSharedLinks());
				for (auto& link : projectDepends)
				{
					List::addIfDoesNotExist(dependsList[name], std::move(link));
				}
			}

			lastDependencies.emplace_back(target->name());
			lastDependencyWasSource = target->isSources();
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

		auto& lastTarget = debugState.inputs.lastTarget();

		for (auto& [name, depends] : dependsList)
		{
			node.addElement("Project", [this, &name, &depends, &lastTarget](XmlElement& node2) {
				node2.addAttribute("filename", getRelativeProjectPath(name));

				if (!lastTarget.empty() && String::equals(lastTarget, name))
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
	auto& lastTarget = debugState.inputs.lastTarget();

	std::string activeProject;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			if (!lastTarget.empty() && String::equals(lastTarget, target->name()))
			{
				activeProject = target->name();
				break;
			}
		}

		if (!activeProject.empty())
			break;
	}

	xmlRoot.setName("CodeBlocks_workspace_layout_file");
	if (!activeProject.empty())
	{
		xmlRoot.addElement("ActiveProject", [this, &activeProject](XmlElement& node) {
			node.addAttribute("path", getRelativeProjectPath(activeProject));
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
