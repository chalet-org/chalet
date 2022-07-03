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
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"

namespace chalet
{
/*****************************************************************************/
VSSolutionProjectExporter::VSSolutionProjectExporter(CentralState& inCentralState) :
	IProjectExporter(inCentralState, ExportKind::VisualStudioSolution)
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
		Diagnostic::error("{} exporter requires the Visual Studio toolchain (set with --toolchain/-t).", typeName);
#else
		Diagnostic::error("{} exporter requires the Visual Studio toolchain on Windows.", typeName);
#endif
		return false;
	}

	return true;
}

/*****************************************************************************/
bool VSSolutionProjectExporter::generateProjectFiles()
{
	if (!useExportDirectory("vs-solution"))
		return false;

	const BuildState* state = getAnyBuildStateButPreferDebug();
	chalet_assert(state != nullptr, "");
	if (state != nullptr)
	{
		UNUSED(state);

		const auto& workspaceName = state->workspace.metadata().name();

		VSSolutionGen slnGen(m_states, m_cwd);
		if (!slnGen.saveToFile(fmt::format("{}.sln", workspaceName)))
		{
			Diagnostic::error("There was a problem saving the {}.sln file.", workspaceName);
			return false;
		}

		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				VSVCXProjGen vcxprojGen(m_states, m_cwd);
				if (!vcxprojGen.saveToFile(target->name()))
				{
					Diagnostic::error("There was a problem saving the {}.vcxproj file.", target->name());
					return false;
				}
			}
		}
	}

	return true;
}
}
