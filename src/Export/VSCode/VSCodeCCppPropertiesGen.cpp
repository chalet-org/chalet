/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeCCppPropertiesGen.hpp"

#include "BuildEnvironment/BuildEnvironmentEmscripten.hpp"
#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Platform.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeCCppPropertiesGen::VSCodeCCppPropertiesGen(const BuildState& inState, const ExportAdapter& inExportAdapter) :
	m_state(inState),
	m_exportAdapter(inExportAdapter)
{
}

/*****************************************************************************/
bool VSCodeCCppPropertiesGen::saveToFile(const std::string& inFilename) const
{
	std::string cStandard;
	std::string cppStandard;
	StringList defines = getPlatformDefines();
	StringList includePath;
	StringList forcedInclude;
#if defined(CHALET_MACOS)
	StringList macFrameworkPath{
		"/System/Library/Frameworks",
		"/Library/Frameworks",
	};
#endif

	auto cwd = Path::getWithSeparatorSuffix(m_state.inputs.workingDirectory());

	bool hasProjects = false;
	for (auto& target : m_state.targets)
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
				Path::toUnix(path);

				auto canonical = Files::getCanonicalPath(path);
				if (String::startsWith(cwd, canonical))
				{
					m_state.inputs.clearWorkingDirectory(canonical);
					path = fmt::format("${{workspaceFolder}}/{}", canonical);
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
					m_state.inputs.clearWorkingDirectory(canonical);
					path = fmt::format("${{workspaceFolder}}/{}", canonical);
				}

				List::addIfDoesNotExist(includePath, path);
			}

			if (String::equals(String::toLowerCase(project.executionCharset()), "utf-8"))
			{
				List::addIfDoesNotExist(defines, std::string("_UNICODE"));
				List::addIfDoesNotExist(defines, std::string("UNICODE"));
			}

			for (const auto& define : project.defines())
			{
				List::addIfDoesNotExist(defines, define);
			}

#if defined(CHALET_MACOS)
			for (auto path : project.appleFrameworkPaths())
			{
				if (path.back() == '/')
					path.pop_back();

				List::addIfDoesNotExist(macFrameworkPath, path);
			}
#endif

			hasProjects = true;
		}
	}

	addSystemIncludes(includePath);

	if (!hasProjects)
		return true;

	// JSON

	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = 4;
	jRoot["configurations"] = Json::array();

	Json config;
	config["name"] = getName();

	if (!m_state.environment->isEmscripten())
	{
		config["intelliSenseMode"] = getIntellisenseMode();
	}

	config["compilerPath"] = getCompilerPath();

	if (m_state.info.generateCompileCommands() && !m_state.environment->isEmscripten())
	{
		m_exportAdapter.createCompileCommandsStub();

		auto outputDirectory = m_state.inputs.outputDirectory();
		m_state.inputs.clearWorkingDirectory(outputDirectory);
		config["compileCommands"] = fmt::format("${{workspaceFolder}}/{}/compile_commands.json", outputDirectory);
	}

	if (!cStandard.empty())
		config["cStandard"] = std::move(cStandard);

	if (!cppStandard.empty())
		config["cppStandard"] = std::move(cppStandard);

	config["defines"] = std::move(defines);
	config["forcedInclude"] = std::move(forcedInclude);
	config["includePath"] = std::move(includePath);
#if defined(CHALET_MACOS)
	config["macFrameworkPath"] = std::move(macFrameworkPath);
#endif

	jRoot["configurations"].push_back(std::move(config));

	return JsonFile::saveToFile(jRoot, inFilename, 1);
}

/*****************************************************************************/
std::string VSCodeCCppPropertiesGen::getName() const
{
	if (m_state.environment->isEmscripten())
	{
		return "Emscripten";
	}

#if defined(CHALET_WIN32)
	return "Win32";
#elif defined(CHALET_MACOS)
	return "Mac";
#else
	return "Linux";
#endif
}

/*****************************************************************************/
std::string VSCodeCCppPropertiesGen::getIntellisenseMode() const
{
	auto platform = Platform::platform();
	auto toolchain = m_state.environment->getCompilerAliasForVisualStudio();

	auto arch = Arch::toVSArch(m_state.info.targetArchitecture());
	return fmt::format("{}-{}-{}", platform, toolchain, arch);
}

/*****************************************************************************/
std::string VSCodeCCppPropertiesGen::getCompilerPath() const
{
	if (m_state.environment->isEmscripten())
	{
		auto& environment = static_cast<BuildEnvironmentEmscripten&>(*m_state.environment);
		return environment.clangPath();
	}

	std::string ret;

	if (!m_state.toolchain.compilerCpp().path.empty())
		ret = m_state.toolchain.compilerCpp().path;
	else
		ret = m_state.toolchain.compilerC().path;

#if defined(CHALET_MACOS)
	const auto& xcodePath = Files::getXcodePath();
	String::replaceAll(ret, xcodePath, "");
	String::replaceAll(ret, "/Toolchains/XcodeDefault.xctoolchain", "");
#endif

	return ret;
}

/*****************************************************************************/
StringList VSCodeCCppPropertiesGen::getPlatformDefines() const
{
	if (m_state.environment->isEmscripten())
	{
		return StringList{
			"__EMSCRIPTEN__"
		};
	}

	return Platform::getDefaultPlatformDefines();
}

/*****************************************************************************/
void VSCodeCCppPropertiesGen::addSystemIncludes(StringList& outList) const
{
	if (m_state.environment->isEmscripten())
	{
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/cache/sysroot/include");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/lib/libc/musl/include");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/lib/libc/musl/arch/emscripten");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/lib/libc/compat");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/lib/libcxx/include");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/lib/libcxxabi/include");
		outList.emplace_back("${env:EMSDK}/upstream/emscripten/system/include");
	}
}

}
