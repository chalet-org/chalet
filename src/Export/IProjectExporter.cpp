/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/IProjectExporter.hpp"

#include "Builder/ConfigureFileParser.hpp"
#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/CLionProjectExporter.hpp"
#include "Export/CodeBlocksProjectExporter.hpp"
#include "Export/FleetProjectExporter.hpp"
#include "Export/VSCodeProjectExporter.hpp"
#include "Export/VSJsonProjectExporter.hpp"
#include "Export/VSSolutionProjectExporter.hpp"
#include "Export/XcodeProjectExporter.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonValues.hpp"

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
		case ExportKind::Xcode:
			return std::make_unique<XcodeProjectExporter>(inInputs);
		case ExportKind::CLion:
			return std::make_unique<CLionProjectExporter>(inInputs);
		case ExportKind::Fleet:
			return std::make_unique<FleetProjectExporter>(inInputs);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented ProjectExporter requested: {}", static_cast<i32>(inKind));
	return nullptr;
}

/*****************************************************************************/
std::string IProjectExporter::getProjectBuildFolder(const CommandLineInputs& inInputs)
{
	// return fmt::format("{}/.project", inInputs.outputDirectory());
	return inInputs.outputDirectory();
}

/*****************************************************************************/
ExportKind IProjectExporter::kind() const noexcept
{
	return m_kind;
}
/*****************************************************************************/
std::string IProjectExporter::getAllBuildTargetName() const
{
	return std::string(Values::All);
}

/*****************************************************************************/
bool IProjectExporter::shouldCleanOnReExport() const
{
	return true;
}

/*****************************************************************************/
const std::string& IProjectExporter::workingDirectory() const noexcept
{
	return m_inputs.workingDirectory();
}

/*****************************************************************************/
bool IProjectExporter::saveSchemasToDirectory(const std::string& inDirectory) const
{
	if (!Files::pathExists(inDirectory))
		Files::makeDirectory(inDirectory);

	// Generate schemas
	{
		Json schema = ChaletJsonSchema::get(m_inputs);
		if (!JsonFile::saveToFile(schema, fmt::format("{}/chalet.schema.json", inDirectory), -1))
			return false;
	}
	{
		Json schema = SettingsJsonSchema::get(m_inputs);
		if (!JsonFile::saveToFile(schema, fmt::format("{}/chalet-settings.schema.json", inDirectory), -1))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool IProjectExporter::useDirectory(const std::string& inDirectory)
{
	if (inDirectory.empty())
		return false;

	if (!Files::pathExists(inDirectory))
	{
		if (!Files::makeDirectory(inDirectory))
		{
			Diagnostic::error("There was a problem creating the '{}' directory.", inDirectory);
			return false;
		}
	}

	m_directory = fmt::format("{}/{}", workingDirectory(), inDirectory);

	// Note: Exported projects should be cleaned if they don't have a build strategy
	//
	if (shouldCleanOnReExport())
	{
		cleanExportDirectory();
	}

	return true;
}

/*****************************************************************************/
bool IProjectExporter::useProjectBuildDirectory(const std::string& inSubDirectory)
{
	auto directory = getProjectBuildFolder(m_inputs);
	if (!inSubDirectory.empty())
		directory += fmt::format("/{}", inSubDirectory);

	return useDirectory(directory);
}

/*****************************************************************************/
void IProjectExporter::cleanExportDirectory()
{
	// Wipe the old one
	if (!m_directory.empty() && Files::pathExists(m_directory))
		Files::removeRecursively(m_directory);
}

/*****************************************************************************/
bool IProjectExporter::generate(CentralState& inCentralState, const bool inForBuild)
{
	Timer timer;

	Output::setShowCommandOverride(false);

	bool added = false;
	if (inForBuild && !makeStateAndValidate(inCentralState, inCentralState.inputs().buildConfiguration(), added))
		return false;

	auto& cacheFile = inCentralState.cache.file();
	const bool exportPathExists = inForBuild && Files::pathExists(getMainProjectOutput());
	const bool appVersionChanged = cacheFile.appVersionChanged();
	const bool buildFileChanged = cacheFile.buildFileChanged();
	const bool buildHashChanged = cacheFile.buildHashChanged();
	const bool requiresRegen = !exportPathExists || appVersionChanged || buildFileChanged || buildHashChanged;
	const bool willGenerate = !inForBuild || requiresRegen;

	auto projectType = getProjectTypeName();
	if (willGenerate)
	{
		if (inForBuild)
			Diagnostic::infoEllipsis("Generating '{}' format", projectType);
		else
			Diagnostic::infoEllipsis("Exporting to '{}' project format", projectType);

		if (!inCentralState.tools.isSigningIdentityValid())
			return false;

		if (!generateStatesAndValidate(inCentralState))
		{
			Diagnostic::error("There was a problem initializing the project exporter.");
			return false;
		}

		if (!makeExportAdapter())
		{
			Diagnostic::error("There was a problem initializing the project exporter.");
			return false;
		}

		if (!validateDebugState())
		{
			Diagnostic::error("There was a problem validating the project exporter.");
			return false;
		}

		if (!generateProjectFiles())
			return false;

		Diagnostic::printDone(timer.asString());
	}

	Output::setShowCommandOverride(true);

	const auto& inputs = inCentralState.inputs();
	if (inputs.route().isExport())
	{
		const auto color = Output::getAnsiStyle(Output::theme().build);
		const auto flair = Output::getAnsiStyle(Output::theme().flair);
		const auto reset = Output::getAnsiStyle(Output::theme().reset);

		auto directory = IProjectExporter::getProjectBuildFolder(inputs);
		auto project = getMainProjectOutput();
		const auto& cwd = inputs.workingDirectory();
		String::replaceAll(project, fmt::format("{}/", cwd), "");

		auto output = fmt::format("\n   Output {}\u2192 {}{}{}\n", flair, color, project, reset);
		std::cout.write(output.data(), output.size());
		std::cout.flush();

		if (inputs.openAfterExport())
		{
			if (!openProjectFilesInEditor(project))
			{
				Diagnostic::error("There was a problem opening the editor.");
				return false;
			}
		}
		else
		{
			Diagnostic::stepInfo("{}Note: Use '--open' with this command to open in {}.{}", flair, projectType, reset);
		}
	}

	return true;
}

/*****************************************************************************/
bool IProjectExporter::generateStatesAndValidate(CentralState& inCentralState)
{
	m_states.clear();

	const auto& configMap = inCentralState.buildConfigurations();

	StringList buildConfigurations = m_inputs.exportBuildConfigurations();
	if (buildConfigurations.empty())
	{
		for (const auto& [name, _] : configMap)
		{
			buildConfigurations.emplace_back(name);
		}
	}

	bool result = true;
	for (auto& name : buildConfigurations)
	{
		if (configMap.find(name) == configMap.end())
		{
			Diagnostic::error("Build configuration not found for this project: {}", name);
			result = false;
		}
	}

	if (!result)
		return false;

	for (auto& name : buildConfigurations)
	{
		auto& config = configMap.at(name);

		// skip configurations with sanitizers for now
		// if (config.enableSanitizers())
		// 	continue;

		if (m_debugConfiguration.empty() && config.debugSymbols())
			m_debugConfiguration = name;

		bool added = false;
		if (!makeStateAndValidate(inCentralState, name, added))
			return false;

		if (!added)
			continue;
	}

	if (m_states.empty())
	{
		Diagnostic::error("There are no valid projects to export.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool IProjectExporter::makeStateAndValidate(CentralState& inCentralState, const std::string& configName, bool& added)
{
	for (auto& state : m_states)
	{
		if (String::equals(state->configuration.name(), configName))
			return true;
	}

	bool quiet = Output::quietNonBuild();
	Output::setQuietNonBuild(true);

	CommandLineInputs inputs = m_inputs;
	inputs.setBuildConfiguration(std::string(configName));
	inputs.setRoute(CommandRoute(RouteType::Export));

	auto state = std::make_unique<BuildState>(std::move(inputs), inCentralState);
	state->setCacheEnabled(false);
	if (!state->initialize())
	{
		if (!state->isBuildConfigurationSupported())
		{
			Output::setQuietNonBuild(quiet);
			return true;
		}

		return false;
	}

	for (auto& target : state->targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);

			// Generate the configure files upfront - TODO: kind of a brittle solution
			//
			if (!project.configureFiles().empty())
			{
				auto outFolder = state->paths.intermediateDir();
				// if (m_kind == ExportKind::Xcode)
				// {
				// 	outFolder = fmt::format("{}/obj.{}", state->paths.buildOutputDir(), project.name());
				// }
				ConfigureFileParser confFileParser(*state, project);
				if (!confFileParser.run(outFolder))
					return false;
			}
		}
	}

	m_states.emplace_back(std::move(state));
	added = true;

	Output::setQuietNonBuild(quiet);

	return true;
}

/*****************************************************************************/
bool IProjectExporter::makeExportAdapter()
{
	m_exportAdapter = std::make_unique<ExportAdapter>(m_states, m_debugConfiguration, getAllBuildTargetName());
	if (!m_exportAdapter->initialize())
		return false;

	return true;
}

/*****************************************************************************/
bool IProjectExporter::validateDebugState()
{
	auto& debugState = m_exportAdapter->getDebugState();
	if (!validate(debugState))
		return false;

	return true;
}
}
