/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/IProjectExporter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Export/CodeBlocksProjectExporter.hpp"
#include "Export/VSCodeProjectExporter.hpp"
#include "Export/VSJsonProjectExporter.hpp"
#include "Export/VSSolutionProjectExporter.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
IProjectExporter::IProjectExporter(const CommandLineInputs& inInputs, const ExportKind inKind) :
	m_inputs(inInputs),
	m_kind(inKind)
{
}

/*****************************************************************************/
IProjectExporter::~IProjectExporter() = default;

/*****************************************************************************/
[[nodiscard]] ProjectExporter IProjectExporter::make(const ExportKind inKind, const CommandLineInputs& inInputs)
{
	switch (inKind)
	{
		case ExportKind::CodeBlocks:
			return std::make_unique<CodeBlocksProjectExporter>(inInputs);
		case ExportKind::VisualStudioCodeJSON:
			return std::make_unique<VSCodeProjectExporter>(inInputs);
		case ExportKind::VisualStudioSolution:
			return std::make_unique<VSSolutionProjectExporter>(inInputs);
		case ExportKind::VisualStudioJSON:
			return std::make_unique<VSJsonProjectExporter>(inInputs);
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
const std::string& IProjectExporter::workingDirectory() const noexcept
{
	return m_inputs.workingDirectory();
}

/*****************************************************************************/
bool IProjectExporter::useDirectory(const std::string& inDirectory)
{
	if (!Commands::pathExists(inDirectory))
	{
		if (!Commands::makeDirectory(inDirectory))
		{
			Diagnostic::error("There was a creating the '{}' directory.", inDirectory);
			return false;
		}
	}

	m_directory = fmt::format("{}/{}", workingDirectory(), inDirectory);
	return true;
}

/*****************************************************************************/
bool IProjectExporter::useExportDirectory(const std::string& inSubDirectory)
{
	if (!m_directory.empty())
		return true;

	std::string exportDirectory{ "chalet_export" };
	if (!inSubDirectory.empty())
		exportDirectory += fmt::format("/{}", inSubDirectory);

	return useDirectory(exportDirectory);
}

/*****************************************************************************/
const BuildState* IProjectExporter::getAnyBuildStateButPreferDebug() const
{
	const BuildState* ret = nullptr;
	if (!m_states.empty())
	{
		if (!m_debugConfiguration.empty())
		{
			for (const auto& state : m_states)
			{
				if (String::equals(m_debugConfiguration, state->configuration.name()))
				{
					ret = state.get();
					break;
				}
			}
		}
		else
		{
			ret = m_states.front().get();
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
bool IProjectExporter::generate(CentralState& inCentralState, const bool inForBuild)
{
	Timer timer;

	Output::setShowCommandOverride(false);

	auto projectType = getProjectTypeName();
	Diagnostic::infoEllipsis("Exporting to '{}' project format", projectType);

	if (!generateStatesAndValidate(inCentralState))
		return false;

	if (inForBuild)
	{
		if (!useDirectory(fmt::format("{}/.projects", m_inputs.outputDirectory())))
			return false;
	}

	if (!generateProjectFiles())
		return false;

	Diagnostic::printDone(timer.asString());

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool IProjectExporter::generateStatesAndValidate(CentralState& inCentralState)
{
	m_states.clear();

	auto makeState = [this, &inCentralState](const std::string& configName) -> bool {
		CommandLineInputs inputs = m_inputs;
		inputs.setBuildConfiguration(std::string(configName));

		auto state = std::make_unique<BuildState>(std::move(inputs), inCentralState);
		if (!state->initialize())
			return false;

		m_states.emplace_back(std::move(state));
		return true;
	};

	bool quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	for (const auto& [name, config] : inCentralState.buildConfigurations())
	{
		// skip configurations with sanitizers for now
		if (config.enableSanitizers())
			continue;

		if (m_debugConfiguration.empty() && config.debugSymbols())
			m_debugConfiguration = name;

		if (!makeState(name))
			return false;
	}

	Output::setQuietNonBuild(quiet);

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

	if (!validate(*state))
		return false;

	populatePathVariable();

	return true;
}

/*****************************************************************************/
void IProjectExporter::populatePathVariable()
{
	m_pathVariables.clear();

	for (auto& state : m_states)
	{
		StringList paths;
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<SourceTarget&>(*target);
				for (auto& p : project.libDirs())
				{
					List::addIfDoesNotExist(paths, p);
				}
				for (auto& p : project.macosFrameworkPaths())
				{
					List::addIfDoesNotExist(paths, p);
				}
			}
		}

		std::reverse(paths.begin(), paths.end());
		m_pathVariables.emplace(state->configuration.name(), state->workspace.makePathVariable(std::string(), paths));
	}
}
}
