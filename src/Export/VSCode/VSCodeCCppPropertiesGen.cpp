/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VSCode/VSCodeCCppPropertiesGen.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
VSCodeCCppPropertiesGen::VSCodeCCppPropertiesGen(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool VSCodeCCppPropertiesGen::saveToFile(const std::string& inFilename) const
{
	Json jRoot;
	jRoot = Json::object();
	jRoot["version"] = 4;
	jRoot["configurations"] = Json::array();

	Json config;
	config["name"] = getName();
	config["intelliSenseMode"] = getIntellisenseMode();
	config["compilerPath"] = getCompilerPath();

	std::string cStandard;
	std::string cppStandard;
	StringList defines = getDefaultDefines();
	StringList includePath;
	StringList forcedInclude;
#if defined(CHALET_MACOS)
	StringList macFrameworkPath{
		"/System/Library/Frameworks",
		"/Library/Frameworks",
	};
#endif

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
				if (Files::pathExists(path))
				{
					path = fmt::format("${{workspaceFolder}}/{}", path);
				}
				List::addIfDoesNotExist(forcedInclude, path);
			}

			for (auto path : project.includeDirs())
			{
				if (path.back() == '/')
					path.pop_back();

				if (Files::pathExists(path) || String::equals(path, m_state.paths.intermediateDir(project)) || String::equals(path, m_state.paths.objDir()))
				{
					path = fmt::format("${{workspaceFolder}}/{}", path);
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

	if (!hasProjects)
		return false;

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
	std::string ret;

#if defined(CHALET_WIN32)
	ret = "Win32";

	if (m_state.environment->isMingw())
		ret += " (MinGW)";

#elif defined(CHALET_MACOS)
	ret = "Mac";
#else
	ret = "Linux";
#endif

	return ret;
}

/*****************************************************************************/
std::string VSCodeCCppPropertiesGen::getIntellisenseMode() const
{
	std::string platform;
#if defined(CHALET_WIN32)
	platform = "windows";
#elif defined(CHALET_MACOS)
	platform = "macos";
#else
	platform = "linux";
#endif

	std::string toolchain;
	if (m_state.environment->isClang())
		toolchain = "clang";
#if defined(CHALET_WIN32)
	else if (m_state.environment->isMsvc())
		toolchain = "msvc";
#endif
	else
		toolchain = "gcc";

	auto arch = Arch::toVSArch(m_state.info.targetArchitecture());
	return fmt::format("{}-{}-{}", platform, toolchain, arch);
}

/*****************************************************************************/
std::string VSCodeCCppPropertiesGen::getCompilerPath() const
{
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
StringList VSCodeCCppPropertiesGen::getDefaultDefines() const
{
	StringList ret;

#if defined(CHALET_WIN32)
	ret.emplace_back("_WIN32");
#elif defined(CHALET_MACOS)
	ret.emplace_back("__APPLE__");
#else
	ret.emplace_back("__linux__");
#endif

	return ret;
}
}
