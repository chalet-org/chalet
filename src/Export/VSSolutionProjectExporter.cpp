/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSSolutionProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/VisualStudio/VSSolutionGen.hpp"
#include "Export/VisualStudio/VSVCXProjGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
VSSolutionProjectExporter::VSSolutionProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::VisualStudioSolution)
{
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
	if (!useProjectBuildDirectory(".vssolution"))
		return false;

	// Details: https://www.codeproject.com/Reference/720512/List-of-Visual-Studio-Project-Type-GUIDs
	//
	const std::string projectTypeGUID{ "8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942" }; // Visual C++
	auto targetGuids = getTargetGuids(projectTypeGUID);

	const BuildState* firstState = getAnyBuildStateButPreferDebug();
	if (firstState != nullptr)
	{
		VSSolutionGen slnGen(m_states, projectTypeGUID, targetGuids);
		if (!slnGen.saveToFile(fmt::format("{}/project.sln", m_directory)))
		{
			Diagnostic::error("There was a problem saving the project.sln file.");
			return false;
		}
	}
	else
	{
		Diagnostic::error("Could not export: internal error");
		return false;
	}

	StringList allSourceTargets;
	StringList allScriptTargets;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				const auto& name = target->name();
				if (!List::contains(allSourceTargets, name))
					allSourceTargets.emplace_back(name);
			}
			else
			{
				const auto& name = target->name();
				if (!List::contains(allScriptTargets, name))
					allScriptTargets.emplace_back(name);
			}
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

	return true;
}

/*****************************************************************************/
OrderedDictionary<Uuid> VSSolutionProjectExporter::getTargetGuids(const std::string& inProjectTypeGUID) const
{
	OrderedDictionary<Uuid> ret;

	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			const auto& name = target->name();
			if (ret.find(name) == ret.end())
			{
				auto key = fmt::format("{}_{}", static_cast<int>(target->type()), name);
				ret.emplace(name, Uuid::v5(key, inProjectTypeGUID));
			}
		}
	}

	return ret;
}
}
