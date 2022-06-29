/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/VSCppPropertiesGen.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// Reference: https://docs.microsoft.com/en-us/cpp/build/cppproperties-schema-reference?view=msvc-170

namespace chalet
{
/*****************************************************************************/
VSCppPropertiesGen::VSCppPropertiesGen(const std::vector<Unique<BuildState>>& inStates, const std::string& inCwd) :
	m_states(inStates),
	m_cwd(inCwd)
{
	UNUSED(m_cwd);
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
		auto architecture = getVSArchitecture(state->info.targetArchitecture());

		Json config;
		config["name"] = fmt::format("{} / {}", architecture, configName);
		config["intelliSenseMode"] = getIntellisenseMode(*state);

		std::string cStandard;
		std::string cppStandard;
		StringList defines{
			"_WIN32"
		};
		StringList includePath{
			"${env.INCLUDE}",
		};
		StringList forcedInclude;

		bool hasProjects = false;

		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);

				if (cStandard.empty())
					cStandard = project.cStandard();

				if (cppStandard.empty())
					cppStandard = project.cppStandard();

				if (project.usesPrecompiledHeader())
				{
					auto path = project.precompiledHeader();
					if (Commands::pathExists(fmt::format("{}/{}", m_cwd, path)))
					{
						path = fmt::format("${{workspaceRoot}}/{}", path);
					}
					List::addIfDoesNotExist(forcedInclude, path);
				}

				for (auto path : project.includeDirs())
				{
					if (path.back() == '/')
						path.pop_back();

					if (Commands::pathExists(fmt::format("{}/{}", m_cwd, path)) || String::equals(path, state->paths.intermediateDir(project)))
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

		config["compilers"] = Json::object();

		auto& compilers = config.at("compilers");

		if (!cStandard.empty())
		{
			compilers["c"] = Json::object();
			compilers["c"]["path"] = state->toolchain.compilerC().path;
			compilers["c"]["standard"] = std::move(cStandard);
		}

		if (!cppStandard.empty())
		{
			compilers["cpp"] = Json::object();
			compilers["cpp"]["path"] = state->toolchain.compilerCpp().path;
			compilers["cpp"]["standard"] = std::move(cppStandard);
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
std::string VSCppPropertiesGen::getVSArchitecture(Arch::Cpu inCpu) const
{
	switch (inCpu)
	{
		case Arch::Cpu::X86:
			return std::string("x86");
		case Arch::Cpu::ARM:
			return std::string("arm");
		case Arch::Cpu::ARM64:
			return std::string("arm64");
		case Arch::Cpu::X64:
		default:
			return std::string("x64");
	}
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

	auto arch = getVSArchitecture(inState.info.targetArchitecture());

	return fmt::format("{}-{}-{}", platform, toolchain, arch);
}

/*****************************************************************************/
Json VSCppPropertiesGen::getEnvironments(const BuildState& inState) const
{
	Json ret = Json::array();

	auto makeEnvironment = [this, &inState](const char* inName, const std::string& inValue) {
		Json env = Json::object();
		env["namespace"] = "chalet";
		env[inName] = inValue;

		return env;
	};

	ret.emplace_back(makeEnvironment("buildDir", inState.paths.buildOutputDir()));
	ret.emplace_back(makeEnvironment("configuration", inState.configuration.name()));
	ret.emplace_back(makeEnvironment("architecture", getVSArchitecture(inState.info.targetArchitecture())));

	return ret;
}

}
