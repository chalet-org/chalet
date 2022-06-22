/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/IProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CodeBlocksProjectExporter.hpp"
#include "Export/VSCodeProjectExporter.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
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

	// BuildState* stateArchA = nullptr;
	for (auto& [config, state] : m_states)
	{
		if (!state->initialize())
			return false;

		if (!validate(*state))
			return false;
	}

	if (!m_states.empty())
	{
		auto& state = *m_states.begin()->second;
		for (auto& target : state.targets)
		{
			if (target->isSources())
			{
				const auto& project = static_cast<const SourceTarget&>(*target);
				m_headerFiles.emplace(project.name(), project.getHeaderFiles());
			}
		}
	}

	Output::setQuietNonBuild(quiet);

	if (!generateProjectFiles())
		return false;

	Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}
}
