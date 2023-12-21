/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudioJson/VSCppPropertiesGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CompileToolchainController.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Platform.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

// Reference: https://docs.microsoft.com/en-us/cpp/build/cppproperties-schema-reference?view=msvc-170

namespace chalet
{
/*****************************************************************************/
VSCppPropertiesGen::VSCppPropertiesGen(const ExportAdapter& inExportAdapter) :
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSCppPropertiesGen::saveToFile(const std::string& inFilename)
{
	m_runConfigs = m_exportAdapter.getBasicRunConfigs();

	auto& debugState = m_exportAdapter.getDebugState();

	Json jRoot;
	jRoot = Json::object();
	jRoot["environments"] = getGlobalEnvironments(debugState);
	jRoot["configurations"] = Json::array();

	auto cwd = Path::getWithSeparatorSuffix(debugState.inputs.workingDirectory());

	auto& allTarget = m_exportAdapter.allBuildName();
	auto& configurations = jRoot.at("configurations");
	for (auto& runConfig : m_runConfigs)
	{
		if (!String::equals(allTarget, runConfig.name))
			continue;

		Json config;
		config["name"] = m_exportAdapter.getRunConfigLabel(runConfig);
		config["intelliSenseMode"] = getIntellisenseMode(debugState, runConfig.arch);

		StringList defines = Platform::getDefaultPlatformDefines();
		StringList includePath{
			"${env.INCLUDE}",
		};
		StringList forcedInclude;

		bool hasProjects = false;

		auto state = m_exportAdapter.getStateFromRunConfig(runConfig);
		if (state == nullptr)
		{
			Diagnostic::error("An internal error occurred creating: {}", inFilename);
			return false;
		}

		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				if (project.usesPrecompiledHeader())
				{
					auto path = project.precompiledHeader();
					Path::toUnix(path);

					auto canonical = Files::getCanonicalPath(path);
					if (String::startsWith(cwd, canonical))
					{
						state->inputs.clearWorkingDirectory(canonical);
						path = fmt::format("${{workspaceRoot}}/{}", canonical);
					}

					List::addIfDoesNotExist(forcedInclude, path);
				}

				for (auto path : project.includeDirs())
				{
					if (path.back() == '/')
						path.pop_back();

					Path::toUnix(path);

					auto canonical = Files::getCanonicalPath(path);
					if (String::startsWith(cwd, canonical))
					{
						state->inputs.clearWorkingDirectory(canonical);
						path = fmt::format("${{workspaceRoot}}/{}", canonical);
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

		const SourceTarget* signifcantTarget = getSignificantTarget(*state);
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

			auto language = sourceTarget.language();
			if (language == CodeLanguage::C || language == CodeLanguage::CPlusPlus)
			{
				compilers["cpp"] = Json::object();
				compilers["cpp"]["path"] = state->toolchain.compilerCpp().path;

				compilers["c"] = Json::object();
				compilers["c"]["path"] = state->toolchain.compilerC().path;
			}

			auto derivative = sourceTarget.getDefaultSourceType();

			StringList args;
			toolchain->compilerCxx->getCommandOptions(args, derivative);
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
std::string VSCppPropertiesGen::getIntellisenseMode(const BuildState& inState, const std::string& inArch) const
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

	auto platform = Platform::platform();
	if (inState.environment->isGcc())
	{
		platform = "linux";
	}

	auto toolchain = inState.environment->getCompilerAliasForVisualStudio();

	auto arch = Arch::toVSArch(Arch::from(inArch).val);

	return fmt::format("{}-{}-{}", platform, toolchain, arch);
}

/*****************************************************************************/
Json VSCppPropertiesGen::getEnvironments(const BuildState& inState) const
{
	Json ret = Json::array();

	auto runEnvironment = m_exportAdapter.getPathVariableForState(inState);

	ret.emplace_back(makeEnvironmentVariable("runEnvironment", runEnvironment));
	ret.emplace_back(makeEnvironmentVariable("buildDir", inState.paths.buildOutputDir()));
	ret.emplace_back(makeEnvironmentVariable("configuration", inState.configuration.name()));
	ret.emplace_back(makeEnvironmentVariable("architecture", inState.info.targetArchitectureString()));

	return ret;
}

/*****************************************************************************/
Json VSCppPropertiesGen::getGlobalEnvironments(const BuildState& inState) const
{
	Json ret = Json::array();

	ret.emplace_back(makeEnvironmentVariable("externalDir", inState.inputs.externalDirectory()));
	ret.emplace_back(makeEnvironmentVariable("toolchain", inState.inputs.toolchainPreferenceName()));

	return ret;
}

/*****************************************************************************/
Json VSCppPropertiesGen::makeEnvironmentVariable(const char* inName, const std::string& inValue) const
{
	Json env = Json::object();
	env["namespace"] = "chalet";
	env[inName] = inValue;

	return env;
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
