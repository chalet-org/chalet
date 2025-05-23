/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/ExportAdapter.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CompileCommandsGenerator.hpp"
#include "Core/CommandLineInputs.hpp"
#include "DotEnv/DotEnvFileGenerator.hpp"
#include "Process/Environment.hpp"
#include "Query/QueryController.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/MesonTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
/*****************************************************************************/
ExportAdapter::ExportAdapter(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, std::string inAllBuildName) :
	m_states(inStates),
	m_debugConfiguration(inDebugConfig),
	m_allBuildName(std::move(inAllBuildName))
{
}

/*****************************************************************************/
bool ExportAdapter::initialize()
{
	auto& debugState = getDebugState();

	m_cwd = debugState.inputs.workingDirectory();
	m_toolchain = getToolchain();
	m_arches = getArchitectures(m_toolchain);

	bool result = true;
	for (auto& arch : m_invalidArches)
	{
		Diagnostic::error("Architecture not found for this toolchain: {}", arch);
		result = false;
	}

	return result;
}

/*****************************************************************************/
bool ExportAdapter::createCompileCommandsStub() const
{
	auto& debugState = getDebugState();
	auto ccmdsJson = debugState.paths.currentCompileCommands();
	if (!Files::pathExists(ccmdsJson))
	{
		CompileCommandsGenerator ccmdsGen(debugState);
		if (!ccmdsGen.addCompileCommandsStubsFromState())
			return false;

		if (!ccmdsGen.saveStub(ccmdsJson))
			return false;
	}

	return true;
}

/*****************************************************************************/
const StringList& ExportAdapter::arches() const noexcept
{
	return m_arches;
}

/*****************************************************************************/
const std::string& ExportAdapter::toolchain() const noexcept
{
	return m_toolchain;
}

/*****************************************************************************/
const std::string& ExportAdapter::cwd() const noexcept
{
	return m_cwd;
}

/*****************************************************************************/
const std::string& ExportAdapter::debugConfiguration() const noexcept
{
	return m_debugConfiguration;
}

/*****************************************************************************/
const std::string& ExportAdapter::allBuildName() const noexcept
{
	return m_allBuildName;
}

/*****************************************************************************/
std::string ExportAdapter::getDefaultTargetName() const
{
	auto& debugState = getDebugState();
	auto target = debugState.getFirstValidRunTarget();
	if (target == nullptr)
		return std::string();

	ExportRunConfiguration runConfig;
	runConfig.name = target->name();
	runConfig.config = debugState.configuration.name();
	runConfig.arch = debugState.info.hostArchitectureString();

	return getRunConfigLabel(runConfig);
}

/*****************************************************************************/
std::string ExportAdapter::getAllTargetName() const
{
	auto& debugState = getDebugState();

	ExportRunConfiguration runConfig;
	runConfig.name = m_allBuildName;
	runConfig.config = debugState.configuration.name();
	runConfig.arch = debugState.info.targetArchitectureString();

	return getRunConfigLabel(runConfig);
}

/*****************************************************************************/
std::string ExportAdapter::getRunConfigLabel(const ExportRunConfiguration& inRunConfig) const
{
	return fmt::format("{} [{} {}]", inRunConfig.name, getLabelArchitecture(inRunConfig), inRunConfig.config);
}

/*****************************************************************************/
std::string ExportAdapter::getLabelArchitecture(const ExportRunConfiguration& inRunConfig) const
{
	std::string arch = inRunConfig.arch;
	String::replaceAll(arch, "x86_64", "x64");
	String::replaceAll(arch, "i686", "x86");
	return arch;
}

/*****************************************************************************/
std::string ExportAdapter::getRunConfigExec() const
{
	return std::string("chalet");
}

/*****************************************************************************/
StringList ExportAdapter::getRunConfigArguments(const ExportRunConfiguration& inRunConfig, std::string cmd, const bool inWithRun) const
{
	bool isAll = String::equals(m_allBuildName, inRunConfig.name);
	auto required = isAll ? "--no-only-required" : "--only-required";
	if (cmd.empty())
	{
		cmd = isAll ? "build" : "buildrun";
	}

	StringList ret{
		"-c",
		inRunConfig.config,
		"-a",
		inRunConfig.arch,
		"-t",
		m_toolchain,
		required,
		"--generate-compile-commands",
		cmd
	};

	if (inWithRun || String::equals("build", cmd))
		ret.push_back(inRunConfig.name);

	return ret;
}

/*****************************************************************************/
ExportRunConfigurationList ExportAdapter::getBasicRunConfigs() const
{
	ExportRunConfigurationList runConfigs;

	for (auto& state : m_states)
	{
		const auto& config = state->configuration.name();

		for (auto& arch : m_arches)
		{
			for (auto& target : state->targets)
			{
				const auto& targetName = target->name();
				if (target->isSources())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (!project.isExecutable())
						continue;

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;

					runConfigs.emplace_back(std::move(runConfig));
				}
				else if (target->isCMake())
				{
					auto& project = static_cast<const CMakeTarget&>(*target);
					if (project.runExecutable().empty())
						continue;

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;

					runConfigs.emplace_back(std::move(runConfig));
				}
				else if (target->isMeson())
				{
					auto& project = static_cast<const MesonTarget&>(*target);
					if (project.runExecutable().empty())
						continue;

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;

					runConfigs.emplace_back(std::move(runConfig));
				}
			}

			{
				ExportRunConfiguration runConfig;
				runConfig.name = m_allBuildName;
				runConfig.config = config;
				runConfig.arch = arch;
				runConfigs.emplace_back(std::move(runConfig));
			}
		}
	}

	return runConfigs;
}

/*****************************************************************************/
ExportRunConfigurationList ExportAdapter::getFullRunConfigs() const
{
	ExportRunConfigurationList runConfigs;

	for (auto& state : m_states)
	{
		const auto& config = state->configuration.name();
		const auto& runArgumentMap = state->getCentralState().runArgumentMap();

		const auto& thisArch = state->info.targetArchitectureString();
		const auto& thisBuildDir = state->paths.buildOutputDir();

		auto env = DotEnvFileGenerator::make(*state);

		for (auto& arch : m_arches)
		{
			auto buildDir = state->paths.buildOutputDir();
			String::replaceAll(buildDir, thisArch, arch);

			auto path = env.getRunPaths();
			if (!path.empty())
			{
				String::replaceAll(path, thisBuildDir, buildDir);
#if defined(CHALET_WIN32)
				Path::toWindows(path);
#endif
			}
			auto libraryPath = env.getLibraryPath();
			if (!libraryPath.empty())
			{
				String::replaceAll(libraryPath, thisBuildDir, buildDir);
			}
			auto frameworkPath = env.getFrameworkPath();
			if (!frameworkPath.empty())
			{
				String::replaceAll(frameworkPath, thisBuildDir, buildDir);
			}

			for (auto& target : state->targets)
			{
				const auto& targetName = target->name();
				if (target->isSources())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (!project.isExecutable())
						continue;

					auto outputFile = state->paths.getTargetFilename(project);
					String::replaceAll(outputFile, thisBuildDir, buildDir);

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;
					runConfig.outputFile = std::move(outputFile);

					if (runArgumentMap.find(targetName) != runArgumentMap.end())
					{
						auto arguments = runArgumentMap.at(targetName);
						for (auto& dir : arguments)
						{
							state->replaceVariablesInString(dir, target.get());
						}

						runConfig.args = std::move(arguments);
					}

					if (!path.empty())
						runConfig.env.emplace(Environment::getPathKey(), path);

					if (!libraryPath.empty())
						runConfig.env.emplace(Environment::getLibraryPathKey(), libraryPath);

					if (!frameworkPath.empty())
						runConfig.env.emplace(Environment::getFrameworkPathKey(), frameworkPath);

					runConfigs.emplace_back(std::move(runConfig));
				}
				else if (target->isCMake())
				{
					auto& project = static_cast<const CMakeTarget&>(*target);
					if (project.runExecutable().empty())
						continue;

					auto outputFile = state->paths.getTargetFilename(project);
					String::replaceAll(outputFile, thisBuildDir, buildDir);

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;
					runConfig.outputFile = std::move(outputFile);

					if (runArgumentMap.find(targetName) != runArgumentMap.end())
						runConfig.args = runArgumentMap.at(targetName);

					if (!path.empty())
						runConfig.env.emplace(Environment::getPathKey(), path);

					if (!libraryPath.empty())
						runConfig.env.emplace(Environment::getLibraryPathKey(), libraryPath);

					if (!frameworkPath.empty())
						runConfig.env.emplace(Environment::getFrameworkPathKey(), frameworkPath);

					runConfigs.emplace_back(std::move(runConfig));
				}
				else if (target->isMeson())
				{
					auto& project = static_cast<const MesonTarget&>(*target);
					if (project.runExecutable().empty())
						continue;

					auto outputFile = state->paths.getTargetFilename(project);
					String::replaceAll(outputFile, thisBuildDir, buildDir);

					ExportRunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;
					runConfig.outputFile = std::move(outputFile);

					if (runArgumentMap.find(targetName) != runArgumentMap.end())
						runConfig.args = runArgumentMap.at(targetName);

					if (!path.empty())
						runConfig.env.emplace(Environment::getPathKey(), path);

					if (!libraryPath.empty())
						runConfig.env.emplace(Environment::getLibraryPathKey(), libraryPath);

					if (!frameworkPath.empty())
						runConfig.env.emplace(Environment::getFrameworkPathKey(), frameworkPath);

					runConfigs.emplace_back(std::move(runConfig));
				}
			}

			{
				ExportRunConfiguration runConfig;
				runConfig.name = m_allBuildName;
				runConfig.config = config;
				runConfig.arch = arch;
				runConfigs.emplace_back(std::move(runConfig));
			}
		}
	}

	return runConfigs;
}

/*****************************************************************************/
std::string ExportAdapter::getPathVariableForState(const BuildState& inState) const
{
	StringList paths;
	for (auto& target : inState.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			for (auto& p : project.libDirs())
				List::addIfDoesNotExist(paths, p);

			for (auto& p : project.appleFrameworkPaths())
				List::addIfDoesNotExist(paths, p);
		}
	}

	return inState.workspace.makePathVariable(std::string(), paths);
}

/*****************************************************************************/
BuildState& ExportAdapter::getDebugState() const
{
	for (auto& state : m_states)
	{
		if (String::equals(m_debugConfiguration, state->configuration.name()))
			return *state;
	}

	return *m_states.front();
}

/*****************************************************************************/
BuildState* ExportAdapter::getStateFromRunConfig(const ExportRunConfiguration& inRunConfig) const
{
	// Note: only reference the configuration here
	for (auto& state : m_states)
	{
		auto& configuration = state->configuration.name();
		if (String::equals(inRunConfig.config, configuration))
			return state.get();
	}

	chalet_assert(false, "requested non-existent state from run config");
	return nullptr;
}

/*****************************************************************************/
const std::string& ExportAdapter::getToolchain() const
{
	auto& debugState = getDebugState();
	return debugState.inputs.toolchainPreferenceName();
}

/*****************************************************************************/
StringList ExportAdapter::getArchitectures(const std::string& inToolchain) const
{
	auto& debugState = getDebugState();
	auto& centralState = debugState.getCentralState();
	const auto& exportArchitectures = debugState.inputs.exportArchitectures();

	StringList excludes{ "auto" };
#if defined(CHALET_WIN32)
	if (debugState.environment->isMsvc())
	{
		excludes.emplace_back("x64_x64");
		excludes.emplace_back("x64_x86");
		excludes.emplace_back("x64_arm64");
		excludes.emplace_back("x64_arm");
		excludes.emplace_back("x86_x86");
		excludes.emplace_back("x86_x64");
		excludes.emplace_back("x86_arm64");
		excludes.emplace_back("x86_arm");
		excludes.emplace_back("x64");
		excludes.emplace_back("x86");
	}
#endif

	auto& archSuffix = debugState.info.targetArchitectureTripleSuffix();

	StringList ret;
	QueryController query(debugState.getCentralState());
	auto arches = query.getArchitectures(inToolchain);
	for (auto&& arch : arches)
	{
		if (String::equals(excludes, arch))
			continue;

		if (!exportArchitectures.empty() && !List::contains(exportArchitectures, arch))
			continue;

		auto triple = fmt::format("{}{}", arch, archSuffix);
		if (!centralState.isAllowedArchitecture(triple, false))
			continue;

		ret.emplace_back(std::move(arch));
	}

	if (ret.empty())
	{
		ret.emplace_back(debugState.info.hostArchitectureString());
	}

	for (auto& arch : exportArchitectures)
	{
		if (String::equals(Values::Auto, arch))
			continue;

		if (List::contains(ret, arch))
			continue;

		m_invalidArches.emplace_back(arch);
	}

	return ret;
}

}
