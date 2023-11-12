/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocksProjectExporter.hpp"

#include "Compile/CompileToolchainController.hpp"
#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/CodeBlocks/CodeBlocksCBPGen.hpp"
#include "Export/CodeBlocks/CodeBlocksWorkspaceGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksProjectExporter::CodeBlocksProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::CodeBlocks)
{
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getMainProjectOutput(const BuildState& inState)
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".codeblocks"))
			return std::string();
	}

	auto project = getProjectName(inState);
	return fmt::format("{}/{}.workspace", m_directory, project);
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getMainProjectOutput()
{
	chalet_assert(!m_states.empty(), "states were empty getting project name");
	return getMainProjectOutput(*m_states.front());
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectTypeName() const
{
	return std::string("CodeBlocks");
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::validate(const BuildState& inState)
{
	if (!inState.environment->isGcc())
	{
		Diagnostic::error("CodeBlocks project format requires a GCC toolchain (set with --toolchain/-t).");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::generateProjectFiles()
{
	auto workspaceFile = getMainProjectOutput();
	if (workspaceFile.empty())
		return false;

	auto allBuildTargetName = getAllBuildTargetName();

	{
		CodeBlocksWorkspaceGen workspaceGen(m_states, m_debugConfiguration, allBuildTargetName);
		if (!workspaceGen.saveToFile(workspaceFile))
		{
			Diagnostic::error("There was a problem creating the CodeBlocks workspace file.");
			return false;
		}
	}
	{
		auto projectFolder = fmt::format("{}/cbp", String::getPathFolder(workspaceFile));
		CodeBlocksCBPGen projectGen(m_states, allBuildTargetName);
		if (!projectGen.saveProjectFiles(projectFolder))
		{
			Diagnostic::error("There was a problem generating the CodeBlocks project files.");
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectName(const BuildState& inState) const
{
	const auto& workspaceName = inState.workspace.metadata().name();
	return !workspaceName.empty() ? workspaceName : std::string("project");
}

}
