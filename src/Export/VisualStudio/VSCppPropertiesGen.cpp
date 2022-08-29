/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/VSCppPropertiesGen.hpp"

#include "Compile/CompileToolchainController.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// Reference: https://docs.microsoft.com/en-us/cpp/build/cppproperties-schema-reference?view=msvc-170

namespace chalet
{
/*****************************************************************************/
VSCppPropertiesGen::VSCppPropertiesGen(const std::vector<Unique<BuildState>>& inStates, const Dictionary<std::string>& inPathVariables) :
	m_states(inStates),
	m_pathVariables(inPathVariables)
{
}

/*****************************************************************************/
bool VSCppPropertiesGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["configurations"] = Json::array();

	auto& configurations = jRoot.at("configurations");

	for (auto& state : m_states)
	{
		const auto& configName = state->configuration.name();
		auto architecture = Arch::toVSArch(state->info.targetArchitecture());

		Json config;
		config["name"] = fmt::format("{} / {}", architecture, configName);
		config["intelliSenseMode"] = getIntellisenseMode(*state);

		StringList defines{
			"_WIN32"
		};
		StringList includePath{
			"${env.INCLUDE}",
		};
		StringList forcedInclude;

		bool hasProjects = false;
		const SourceTarget* signifcantTarget = getSignificantTarget(*state);

		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				if (project.usesPrecompiledHeader())
				{
					auto path = project.precompiledHeader();
					if (Commands::pathExists(path))
					{
						path = fmt::format("${{workspaceRoot}}/{}", path);
					}
					List::addIfDoesNotExist(forcedInclude, path);
				}

				for (auto path : project.includeDirs())
				{
					if (path.back() == '/')
						path.pop_back();

					if (Commands::pathExists(path) || String::equals(path, state->paths.intermediateDir(project)))
					{
						path = fmt::format("${{workspaceRoot}}/{}", path);
					}

					List::addIfDoesNotExist(includePath, path);
				}

				for (const auto& define : project.defines())
				{
					List::addIfDoesNotExist(defines, define);
				}

				if (String::equals(String::toLowerCase(project.executionCharset()), "utf-8"))
				{
					List::addIfDoesNotExist(defines, std::string("UNICODE"));
					List::addIfDoesNotExist(defines, std::string("_UNICODE"));
				}

				hasProjects = true;
			}
		}

		if (!hasProjects)
			return false;

		if (signifcantTarget != nullptr)
		{
			const auto& sourceTarget = *signifcantTarget;
			auto toolchain = std::make_unique<CompileToolchainController>(sourceTarget);
			if (!toolchain->initialize(*state))
			{
				Diagnostic::error("Error preparing the toolchain for project: {}", sourceTarget.name());
				return false;
			}

			config["compilers"] = Json::object();

			auto& compilers = config.at("compilers");

			if (!sourceTarget.cStandard().empty())
			{
				compilers["c"] = Json::object();
				compilers["c"]["path"] = state->toolchain.compilerC().path;
			}

			if (!sourceTarget.cppStandard().empty())
			{
				compilers["cpp"] = Json::object();
				compilers["cpp"]["path"] = state->toolchain.compilerCpp().path;
			}

			auto specialization = sourceTarget.cxxSpecialization();

			StringList args;
			toolchain->compilerCxx->getCommandOptions(args, specialization);
			config["compilerSwitches"] = String::join(args);
		}

		config["defines"] = std::move(defines);
		config["forcedInclude"] = std::move(forcedInclude);
		config["includePath"] = std::move(includePath);
		config["environments"] = getEnvironments(*state);

		configurations.emplace_back(std::move(config));
	}

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
std::string VSCppPropertiesGen::getIntellisenseMode(const BuildState& inState) const
{
	/*
		windows-msvc-x86
		windows-msvc-x64
		windows-msvc-arm
		windows-msvc-arm64
		android-clang-x86
		android-clang-x64
		android-clang-arm
		android-clang-arm64
		ios-clang-x86
		ios-clang-x64
		ios-clang-arm
		ios-clang-arm64
		windows-clang-x86
		windows-clang-x64
		windows-clang-arm
		windows-clang-arm64
		linux-gcc-x86
		linux-gcc-x64
		linux-gcc-arm
	*/

	std::string platform{ "windows" };
	if (inState.environment->isGcc())
	{
		platform = "linux";
	}

	std::string toolchain{ "msvc" };
	if (inState.environment->isWindowsClang())
	{
		toolchain = "clang";
	}
	else if (inState.environment->isGcc())
	{
		toolchain = "gcc";
	}

	auto arch = Arch::toVSArch(inState.info.targetArchitecture());

	return fmt::format("{}-{}-{}", platform, toolchain, arch);
}

/*****************************************************************************/
Json VSCppPropertiesGen::getEnvironments(const BuildState& inState) const
{
	Json ret = Json::array();

	auto makeEnvironment = [](const char* inName, const std::string& inValue) {
		Json env = Json::object();
		env["namespace"] = "chalet";
		env[inName] = inValue;

		return env;
	};

	// workspace.makePathVariable(path, inAdditionalPaths);

	const auto& configName = inState.configuration.name();

	chalet_assert(m_pathVariables.find(configName) != m_pathVariables.end(), "");
	const auto& runEnvironment = m_pathVariables.at(configName);

	ret.emplace_back(makeEnvironment("runEnvironment", runEnvironment));
	ret.emplace_back(makeEnvironment("buildDir", inState.paths.buildOutputDir()));
	ret.emplace_back(makeEnvironment("externalDir", inState.inputs.externalDirectory()));
	ret.emplace_back(makeEnvironment("externalBuildDir", inState.paths.externalBuildDir()));
	ret.emplace_back(makeEnvironment("configuration", configName));
	ret.emplace_back(makeEnvironment("vsArch", Arch::toVSArch(inState.info.targetArchitecture())));
	ret.emplace_back(makeEnvironment("architecture", inState.info.targetArchitectureString()));
	ret.emplace_back(makeEnvironment("targetTriple", inState.info.targetArchitectureTriple()));

	return ret;
}

/*****************************************************************************/
// Get the first executable or the last non-executable source target
//
const SourceTarget* VSCppPropertiesGen::getSignificantTarget(const BuildState& inState) const
{
	const SourceTarget* ret = nullptr;

	{
		std::vector<const SourceTarget*> allTargets;
		std::vector<const SourceTarget*> executableTargets;

		for (auto& target : inState.targets)
		{
			if (target->isSources())
			{
				allTargets.emplace_back(static_cast<const SourceTarget*>(target.get()));

				const auto& project = static_cast<const SourceTarget&>(*target);
				if (project.isExecutable())
				{
					executableTargets.emplace_back(static_cast<const SourceTarget*>(target.get()));
				}
			}
		}

		if (executableTargets.empty() && !allTargets.empty())
		{
			return *allTargets.rbegin();
		}
		else
		{
			ret = *executableTargets.begin();
		}
	}

	return ret;
}

}
