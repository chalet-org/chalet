/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/IProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CodeBlocksProjectExporter.hpp"
#include "Export/VSCodeProjectExporter.hpp"
#include "Export/VSJsonProjectExporter.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
IProjectExporter::IProjectExporter(CentralState& inCentralState, const ExportKind inKind) :
	m_centralState(inCentralState),
	m_kind(inKind)
{
}

/*****************************************************************************/
IProjectExporter::~IProjectExporter() = default;

/*****************************************************************************/
[[nodiscard]] ProjectExporter IProjectExporter::make(const ExportKind inKind, CentralState& inCentralState)
{
	switch (inKind)
	{
		case ExportKind::CodeBlocks:
			return std::make_unique<CodeBlocksProjectExporter>(inCentralState);
		case ExportKind::VisualStudioCode:
			return std::make_unique<VSCodeProjectExporter>(inCentralState);
		case ExportKind::VisualStudioJSON:
			return std::make_unique<VSJsonProjectExporter>(inCentralState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ProjectExporter requested: {}", static_cast<int>(inKind));
	return nullptr;
}

/*****************************************************************************/
ExportKind IProjectExporter::kind() const noexcept
{
	return m_kind;
}

/*****************************************************************************/
bool IProjectExporter::useExportDirectory(const std::string& inSubDirectory) const
{
	std::string exportDirectory{ "chalet_export" };
	if (!inSubDirectory.empty())
		exportDirectory += fmt::format("/{}", inSubDirectory);

	if (!Commands::pathExists(exportDirectory))
	{
		if (!Commands::makeDirectory(exportDirectory))
		{
			Diagnostic::error("There was a creating the '{}' directory.", exportDirectory);
			return false;
		}
	}

	if (!Commands::changeWorkingDirectory(fmt::format("{}/{}", m_cwd, exportDirectory)))
	{
		Diagnostic::error("There was a problem changing to the '{}' directory.", exportDirectory);
		return false;
	}

	return true;
}

/*****************************************************************************/
const BuildState* IProjectExporter::getAnyBuildStateButPreferDebug() const
{
	const BuildState* ret = nullptr;
	if (!m_states.empty())
	{
		if (!m_debugConfiguration.empty())
		{
			ret = m_states.at(m_debugConfiguration).get();
		}
		else
		{
			ret = m_states.begin()->second.get();
		}
	}

	return ret;
}

/*****************************************************************************/
const IBuildTarget* IProjectExporter::getRunnableTarget(const BuildState& inState) const
{
	const IBuildTarget* ret = nullptr;
	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
			{
				ret = target.get();
				break;
			}
		}
		else if (target->isCMake())
		{
			const auto& cmakeProject = static_cast<const CMakeTarget&>(*target);
			if (!cmakeProject.runExecutable().empty())
			{
				ret = target.get();
				break;
			}
		}
	}

	return ret;
}

/*****************************************************************************/
bool IProjectExporter::generate()
{
	Timer timer;

	Output::setShowCommandOverride(false);

	auto projectType = getProjectTypeName();
	Diagnostic::infoEllipsis("Exporting to '{}' project format", projectType);

	m_cwd = m_centralState.inputs().workingDirectory();

	auto makeState = [this](const std::string& configName) {
		// auto configName = fmt::format("{}_{}", arch, inConfig);
		if (m_states.find(configName) == m_states.end())
		{
			CommandLineInputs inputs = m_centralState.inputs();
			inputs.setBuildConfiguration(std::string(configName));
			// inputs.setTargetArchitecture(arch);
			auto state = std::make_unique<BuildState>(std::move(inputs), m_centralState);

			m_states.emplace(configName, std::move(state));
		}
	};

	for (const auto& [name, config] : m_centralState.buildConfigurations())
	{
		// skip configurations with sanitizers for now
		if (config.enableSanitizers())
			continue;

		if (m_debugConfiguration.empty() && config.debugSymbols())
			m_debugConfiguration = name;

		makeState(name);
	}

	bool quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	for (auto& [config, state] : m_states)
	{
		if (!state->initialize())
			return false;
	}

	auto state = getAnyBuildStateButPreferDebug();
	if (state == nullptr)
	{
		Diagnostic::error("There are no valid projects to export.");
		return false;
	}

	for (auto& target : state->targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			m_headerFiles.emplace(project.name(), project.getHeaderFiles());
		}
	}

	Output::setQuietNonBuild(quiet);

	if (!validate(*state))
		return false;

	if (!generateProjectFiles())
	{
		Commands::changeWorkingDirectory(m_cwd);
		return false;
	}

	Commands::changeWorkingDirectory(m_cwd);
	Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}
}
