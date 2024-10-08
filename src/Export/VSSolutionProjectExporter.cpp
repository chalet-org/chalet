/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSSolutionProjectExporter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/VisualStudio/VSSolutionGen.hpp"
#include "Export/VisualStudio/VSVCXProjGen.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
VSSolutionProjectExporter::VSSolutionProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::VisualStudioSolution)
{
}

/*****************************************************************************/
std::string VSSolutionProjectExporter::getMainProjectOutput(const BuildState& inState)
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".vssolution"))
			return std::string();
	}

	auto project = getProjectName(inState);
	return fmt::format("{}/{}.sln", m_directory, project);
}

/*****************************************************************************/
std::string VSSolutionProjectExporter::getMainProjectOutput()
{
	chalet_assert(!m_states.empty(), "states were empty getting project name");
	if (m_states.empty())
		return std::string();

	return getMainProjectOutput(*m_states.front());
}

/*****************************************************************************/
std::string VSSolutionProjectExporter::getProjectTypeName() const
{
	return std::string("Visual Studio Solution");
}

/*****************************************************************************/
bool VSSolutionProjectExporter::validate(const BuildState& inState)
{
	auto typeName = getProjectTypeName();
	if (!inState.environment->isMsvc())
	{
#if defined(CHALET_WIN32)
		Diagnostic::error("{} project format requires the Visual Studio toolchain (set with --toolchain/-t).", typeName);
#else
		Diagnostic::error("{} project format requires the Visual Studio toolchain on Windows.", typeName);
#endif
		return false;
	}
	UNUSED(inState);

	return true;
}

/*****************************************************************************/
bool VSSolutionProjectExporter::generateProjectFiles()
{
	auto solution = getMainProjectOutput();
	if (solution.empty())
		return false;

	auto& debugState = m_exportAdapter->getDebugState();

	{
		auto& centralState = debugState.getCentralState();
		const auto& baseArch = debugState.info.targetArchitectureString();
		const auto& arches = m_exportAdapter->arches();

		auto oldStates = std::move(m_states);
		for (auto&& statePtr : oldStates)
		{
			if (String::equals(baseArch, statePtr->info.targetArchitectureString()))
			{
				auto state = statePtr.get();
				const auto& buildConfig = state->inputs.buildConfiguration();
				for (auto& arch : arches)
				{
					if (String::equals(baseArch, arch))
					{
						m_states.emplace_back(std::move(statePtr));
					}
					else
					{
						bool added = false;
						if (!makeStateAndValidate(centralState, arch, buildConfig, added))
							return false;

						if (!added)
						{
							Diagnostic::error("Internal error adding: {} / {}", buildConfig, arch);
							return false;
						}
					}
				}

				// baseStates.emplace_back(state.get());
			}
		}
	}

	auto allBuildTargetName = getAllBuildTargetName();

	// Details: https://www.codeproject.com/Reference/720512/List-of-Visual-Studio-Project-Type-GUIDs
	//
	const std::string projectTypeGUID{ "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" }; // Visual C++
	auto targetGuids = getTargetGuids(projectTypeGUID, allBuildTargetName);

	VSSolutionGen slnGen(*m_exportAdapter, projectTypeGUID, targetGuids);
	if (!slnGen.saveToFile(solution))
	{
		auto project = getProjectName(debugState);
		Diagnostic::error("There was a problem saving the {}.sln file.", project);
		return false;
	}

	StringList allSourceTargets;
	StringList allScriptTargets;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			const auto& name = target->name();
			if (target->isSources())
				List::addIfDoesNotExist(allSourceTargets, name);
			else
				List::addIfDoesNotExist(allScriptTargets, name);
		}
	}

	for (auto& name : allSourceTargets)
	{
		VSVCXProjGen vcxprojGen(m_states, m_directory, projectTypeGUID, targetGuids);
		if (!vcxprojGen.saveSourceTargetProjectFiles(name))
		{
			Diagnostic::error("There was a problem saving the {}.vcxproj file.", name);
			return false;
		}
	}
	if (!allScriptTargets.empty())
	{
		for (auto& name : allScriptTargets)
		{
			VSVCXProjGen vcxprojGen(m_states, m_directory, projectTypeGUID, targetGuids);
			if (!vcxprojGen.saveScriptTargetProjectFiles(name))
			{
				Diagnostic::error("There was a problem saving the {}.vcxproj file.", name);
				return false;
			}
		}
	}
	// all_build
	{
		VSVCXProjGen vcxprojGen(m_states, m_directory, projectTypeGUID, targetGuids);
		if (!vcxprojGen.saveAllBuildTargetProjectFiles(allBuildTargetName))
		{
			Diagnostic::error("There was a problem saving the {}.vcxproj file.", allBuildTargetName);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool VSSolutionProjectExporter::openProjectFilesInEditor(const std::string& inProject)
{
	std::string devEnvDir;
	auto visualStudio = Files::which("devenv");
	if (visualStudio.empty())
	{
		devEnvDir = Environment::getString("DevEnvDir");
		visualStudio = fmt::format("{}\\devenv.exe", devEnvDir);
		if (devEnvDir.empty() || !Files::pathExists(visualStudio))
		{
			Diagnostic::error("Failed to launch in Visual Studio: {}", inProject);
			return false;
		}
	}
	else
	{
		devEnvDir = String::getPathFolder(visualStudio);
	}

	auto project = Files::getCanonicalPath(inProject);
	return Process::runMinimalOutputWithoutWait({ visualStudio, project }, devEnvDir);
}

/*****************************************************************************/
OrderedDictionary<Uuid> VSSolutionProjectExporter::getTargetGuids(const std::string& inProjectTypeGUID, const std::string& inAllBuildName) const
{
	OrderedDictionary<Uuid> ret;

	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			const auto& name = target->name();
			if (ret.find(name) == ret.end())
			{
				auto key = fmt::format("{}_{}", static_cast<i32>(target->type()), name);
				ret.emplace(name, Uuid::v5(key, inProjectTypeGUID));
			}
		}
	}

	if (!inAllBuildName.empty())
	{
		auto key = fmt::format("{}_{}", std::numeric_limits<i32>::max(), inAllBuildName);
		ret.emplace(inAllBuildName, Uuid::v5(key, inProjectTypeGUID));
	}

	return ret;
}

/*****************************************************************************/
std::string VSSolutionProjectExporter::getProjectName(const BuildState& inState) const
{
	const auto& workspaceName = inState.workspace.metadata().name();
	return !workspaceName.empty() ? workspaceName : std::string("project");
}

/*****************************************************************************/
bool VSSolutionProjectExporter::shouldCleanOnReExport() const
{
	return false;
}
/*****************************************************************************/
bool VSSolutionProjectExporter::requiresConfigureFiles() const
{
	return true;
}
}
