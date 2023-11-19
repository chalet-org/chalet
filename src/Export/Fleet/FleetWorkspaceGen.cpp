/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Fleet/FleetWorkspaceGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Query/QueryController.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
FleetWorkspaceGen::FleetWorkspaceGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inDebugConfig, const std::string& inAllBuildName) :
	m_states(inStates),
	m_debugConfiguration(inDebugConfig),
	m_allBuildName(inAllBuildName)
{
}

/*****************************************************************************/
bool FleetWorkspaceGen::saveToPath(const std::string& inPath)
{
	UNUSED(m_states, m_debugConfiguration, m_allBuildName);

	auto& debugState = getDebugState();

	m_cwd = debugState.inputs.workingDirectory();
	m_toolchain = getToolchain();
	m_arches = getArchitectures(m_toolchain);

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

					RunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;

					m_runConfigs.emplace_back(std::move(runConfig));
				}
				else if (target->isCMake())
				{
					auto& project = static_cast<const CMakeTarget&>(*target);
					if (project.runExecutable().empty())
						continue;

					RunConfiguration runConfig;
					runConfig.name = targetName;
					runConfig.config = config;
					runConfig.arch = arch;

					m_runConfigs.emplace_back(std::move(runConfig));
				}
			}

			{
				RunConfiguration runConfig;
				runConfig.name = m_allBuildName;
				runConfig.config = config;
				runConfig.arch = arch;
				m_runConfigs.emplace_back(std::move(runConfig));
			}
		}
	}

	auto runJson = fmt::format("{}/run.json", inPath);
	if (!createRunJsonFile(runJson))
		return false;

	return true;
}

/*****************************************************************************/
bool FleetWorkspaceGen::createRunJsonFile(const std::string& inFilename)
{
	Json jRoot;
	jRoot = Json::object();

	jRoot["configurations"] = Json::array();

	for (auto& runConfig : m_runConfigs)
	{
		jRoot["configurations"].emplace_back(makeRunConfiguration(runConfig));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
BuildState& FleetWorkspaceGen::getDebugState() const
{
	for (auto& state : m_states)
	{
		if (String::equals(m_debugConfiguration, state->configuration.name()))
			return *state;
	}

	return *m_states.front();
}

/*****************************************************************************/
const std::string& FleetWorkspaceGen::getToolchain() const
{
	auto& debugState = getDebugState();
	return debugState.inputs.toolchainPreferenceName();
}

/*****************************************************************************/
StringList FleetWorkspaceGen::getArchitectures(const std::string& inToolchain) const
{
	auto& debugState = getDebugState();

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

	StringList ret;
	QueryController query(debugState.getCentralState());
	auto arches = query.getArchitectures(inToolchain);
	for (auto&& arch : arches)
	{
		if (String::equals(excludes, arch))
			continue;

		ret.emplace_back(std::move(arch));
	}

	if (ret.empty())
	{
		ret.emplace_back(debugState.info.hostArchitectureString());
	}

	return ret;
}

/*****************************************************************************/
Json FleetWorkspaceGen::makeRunConfiguration(const RunConfiguration& inRunConfig) const
{
	Json ret;
	ret["type"] = "command";
	ret["name"] = fmt::format("{} [{} {}]", inRunConfig.name, inRunConfig.arch, inRunConfig.config);
	ret["workingDir"] = m_cwd;

	ret["program"] = "chalet";

	bool isAll = String::equals(m_allBuildName, inRunConfig.name);
	auto required = isAll ? "--no-only-required" : "--only-required";
	auto cmd = isAll ? "build" : "buildrun";
	ret["args"] = {
		"-c",
		inRunConfig.config,
		"-a",
		inRunConfig.arch,
		"-t",
		m_toolchain,
		required,
		"--generate-compile-commands",
		cmd,
		inRunConfig.name
	};

	return ret;
}

}