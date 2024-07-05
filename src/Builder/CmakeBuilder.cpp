/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/CmakeBuilder.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Utility/Version.hpp"

namespace chalet
{
/*****************************************************************************/
CmakeBuilder::CmakeBuilder(const BuildState& inState, const CMakeTarget& inTarget, const bool inQuotedPaths) :
	m_state(inState),
	m_target(inTarget),
	m_quotedPaths(inQuotedPaths)
{
	m_cmakeVersionMajorMinor = m_state.toolchain.cmakeVersionMajor();
	m_cmakeVersionMajorMinor *= 100;
	m_cmakeVersionMajorMinor += m_state.toolchain.cmakeVersionMinor();
}

/*****************************************************************************/
std::string CmakeBuilder::getLocation() const
{
	const auto& rawLocation = m_target.location();
	auto ret = Files::getAbsolutePath(rawLocation);
	Path::toUnix(ret);

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getBuildFile(const bool inForce) const
{
	std::string ret;
	if (!m_target.buildFile().empty())
	{
		auto location = getLocation();
		ret = fmt::format("{}/{}", location, m_target.buildFile());
	}
	else if (inForce)
	{
		auto location = getLocation();
		ret = fmt::format("{}/CMakeLists.txt", location);
	}

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getCacheFile() const
{
	std::string ret;

	auto location = m_target.targetFolder();
	ret = fmt::format("{}/CMakeCache.txt", location);

	return ret;
}

/*****************************************************************************/
bool CmakeBuilder::dependencyHasUpdated() const
{
	if (m_target.hashChanged())
	{
		Files::removeRecursively(m_target.targetFolder());
		return true;
	}

	return false;
}

/*****************************************************************************/
bool CmakeBuilder::run()
{
	Timer buildTimer;

	const auto& name = m_target.name();

	// TODO: add doxygen to path?

	const bool isNinja = usesNinja();

	static const char kNinjaStatus[] = "NINJA_STATUS";
	auto oldNinjaStatus = Environment::getString(kNinjaStatus);

	auto onRunFailure = [this, &oldNinjaStatus, &isNinja](const bool inRemoveDir = true) -> bool {
#if defined(CHALET_WIN32)
		Output::previousLine();
#endif

		if (inRemoveDir && !m_target.recheck())
			Files::removeRecursively(outputLocation());

		Output::lineBreak();

		if (isNinja)
			Environment::set(kNinjaStatus, oldNinjaStatus);

		return false;
	};

	auto& sourceCache = m_state.cache.file().sources();
	auto outputHash = Hash::string(outputLocation());
	bool lastBuildFailed = sourceCache.dataCacheValueIsFalse(outputHash);
	bool dependencyUpdated = dependencyHasUpdated();

	bool outDirectoryDoesNotExist = !Files::pathExists(outputLocation());
	bool recheckCmake = m_target.recheck() || lastBuildFailed || dependencyUpdated;

	if (outDirectoryDoesNotExist || recheckCmake)
	{
		if (outDirectoryDoesNotExist)
			Files::makeDirectory(outputLocation());

		if (isNinja)
		{
			const auto& color = Output::getAnsiStyle(Output::theme().build);
			Environment::set(kNinjaStatus, fmt::format("   [%f/%t] {}", color));
		}

		StringList command;
		command = getGeneratorCommand();

		{
			std::string cwd = m_cmakeVersionMajorMinor >= 313 ? std::string() : outputLocation();
			if (!Process::run(command, cwd))
				return onRunFailure();
		}

		command = getBuildCommand(outputLocation());

		// this will control ninja output, and other build outputs should be unaffected
		bool result = Process::runNinjaBuild(command);
		sourceCache.addDataCache(outputHash, result);
		if (!result)
			return onRunFailure(false);

		if (isNinja)
		{
			Environment::set(kNinjaStatus, oldNinjaStatus);
		}
	}

	//
	Output::msgTargetUpToDate(name, &buildTimer);

	return true;
}

/*****************************************************************************/
std::string CmakeBuilder::getGenerator() const
{
	const bool isNinja = usesNinja();

	std::string ret;
	if (isNinja)
	{
		ret = "Ninja";
	}
#if defined(CHALET_WIN32)
	// The frustrating thing about these is that they always output files in the build configuration folder
	//
	/*else if (m_state.environment->isMsvc())
	{
		// Validated in CMakeTarget::validate
		const auto& version = m_state.toolchain.version();
		if (String::startsWith("17.", version))
		{
			ret = "Visual Studio 17 2022";
		}
		else if (String::startsWith("16.", version))
		{
			ret = "Visual Studio 16 2019";
		}
		else if (String::startsWith("15.", version))
		{
			ret = "Visual Studio 15 2017";
		}
		else if (String::startsWith("14.", version))
		{
			ret = "Visual Studio 14 2015";
		}
		else if (String::startsWith("12.", version))
		{
			ret = "Visual Studio 12 2013";
		}
		else if (String::startsWith("11.", version))
		{
			ret = "Visual Studio 11 2012";
		}
		else if (String::startsWith("10.", version))
		{
			ret = "Visual Studio 10 2010";
		}
	}*/
#endif
	else
	{
#if defined(CHALET_WIN32)
		if (m_state.toolchain.makeIsJom())
		{
			ret = "NMake Makefiles JOM";
		}
		else if (m_state.toolchain.makeIsNMake())
		{
			ret = "NMake Makefiles";
		}
		else
		{
			ret = "MinGW Makefiles";
		}
#else
		ret = "Unix Makefiles";
#endif
	}

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getArchitecture() const
{
	const bool isNinja = usesNinja();

	std::string ret;

	// Note: The -A flag is only really used by VS
	if (!isNinja && m_state.environment->isMsvc())
	{
		switch (m_state.info.targetArchitecture())
		{
			case Arch::Cpu::X86:
				ret = "Win32";
				break;
			case Arch::Cpu::X64:
				ret = "x64";
				break;
			case Arch::Cpu::ARM:
			case Arch::Cpu::ARMHF:
				ret = "ARM";
				break;
			case Arch::Cpu::ARM64:
				ret = "ARM64";
				break;
			default:
				break;
		}
	}

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getGeneratorCommand()
{
	auto location = getLocation();
	auto buildFile = getBuildFile();

	return getGeneratorCommand(location, buildFile);
}

/*****************************************************************************/
StringList CmakeBuilder::getGeneratorCommand(const std::string& inLocation, const std::string& inBuildFile) const
{
	auto& cmake = m_state.toolchain.cmake();

	auto generator = getGenerator();
	chalet_assert(!generator.empty(), "CMake Generator is empty");

	StringList ret{ getQuotedPath(cmake), "-G", getQuotedPath(generator) };

	if (!Output::showCommands())
		ret.emplace_back("--no-warn-unused-cli");

	std::string arch = getArchitecture();
	if (!arch.empty())
	{
		ret.emplace_back("-A");
		ret.emplace_back(std::move(arch));
	}

	if (!inBuildFile.empty())
	{
		ret.emplace_back("-C");
		ret.emplace_back(getQuotedPath(inBuildFile));
	}

	const auto& toolset = m_target.toolset();
	if (!toolset.empty())
	{
		ret.emplace_back("-T");
		ret.emplace_back(getQuotedPath(toolset));
	}

	addCmakeDefines(ret);

	if (m_cmakeVersionMajorMinor >= 313)
	{
		ret.emplace_back("-S");
		ret.emplace_back(getQuotedPath(inLocation));

		auto buildLocation = Files::getAbsolutePath(outputLocation());

		ret.emplace_back("-B");
		ret.emplace_back(getQuotedPath(buildLocation));
	}
	else
	{
		ret.emplace_back(getQuotedPath(inLocation));
	}

	return ret;
}

/*****************************************************************************/
void CmakeBuilder::addCmakeDefines(StringList& outList) const
{
	struct CharMapCompare
	{
		bool operator()(const char* inA, const char* inB) const
		{
			return std::strcmp(inA, inB) < 0;
		}
	};

	StringList checkVariables{
		"CMAKE_WARN_DEPRECATED",
		"CMAKE_EXPORT_COMPILE_COMMANDS",
		"CMAKE_SYSTEM_NAME",
		"CMAKE_SYSTEM_PROCESSOR",
		"CMAKE_CXX_COMPILER",
		"CMAKE_C_COMPILER",
		"CMAKE_RC_COMPILER",
		"CMAKE_CXX_COMPILER_LAUNCHER",
		"CMAKE_BUILD_TYPE",
		"CMAKE_LIBRARY_ARCHITECTURE",
		"CMAKE_LIBRARY_PATH",
		"CMAKE_INCLUDE_PATH",
		"CMAKE_SYSROOT",
		"CMAKE_BUILD_WITH_INSTALL_RPATH",
		"CMAKE_FIND_ROOT_PATH_MODE_PROGRAM",
		"CMAKE_FIND_ROOT_PATH_MODE_LIBRARY",
		"CMAKE_FIND_ROOT_PATH_MODE_INCLUDE",
		"CMAKE_FIND_ROOT_PATH_MODE_PACKAGE",
		"CMAKE_C_COMPILER_TARGET",
		"CMAKE_CXX_COMPILER_TARGET",
		"CMAKE_TOOLCHAIN_FILE",
		"CMAKE_CROSSCOMPILING_EMULATOR",
		"CMAKE_EXECUTABLE_SUFFIX",
#if defined(CHALET_WIN32)
	// "CMAKE_SH",
#elif defined(CHALET_MACOS)
		"CMAKE_OSX_ARCHITECTURES",
#endif
	};

	bool forMsBuild = (m_state.inputs.route().isExport() && m_state.inputs.exportKind() == ExportKind::VisualStudioSolution)
		|| (m_state.toolchain.strategy() == StrategyType::MSBuild);

	std::map<const char*, bool, CharMapCompare> isDefined;
	for (auto& define : m_target.defines())
	{
		auto& lastDefine = outList.emplace_back("-D" + define);
		if (forMsBuild && lastDefine.back() == '\'' && String::contains("='", lastDefine))
		{
			lastDefine.back() = '"';
			String::replaceAll(lastDefine, "='", "=\"");
		}
		else if (!forMsBuild && lastDefine.back() == '"' && String::contains("=\"", lastDefine))
		{
			lastDefine.back() = '\'';
			String::replaceAll(lastDefine, "=\"", "='");
		}

		for (auto& var : checkVariables)
		{
			if (String::contains(var.c_str(), define))
				isDefined[var.c_str()] = true;
		}
	}

	if (!Output::showCommands() && !isDefined["CMAKE_WARN_DEPRECATED"])
	{
		outList.emplace_back("-DCMAKE_WARN_DEPRECATED=OFF");
	}

	const auto& hostTriple = m_state.info.hostArchitectureTriple();
	const auto& targetTriple = m_state.info.targetArchitectureTriple();

	const bool isEmscripten = m_state.environment->isEmscripten();
	const bool usingToolchainFile = isEmscripten;
	const bool crossCompile = !hostTriple.empty() && !String::startsWith(hostTriple, targetTriple) && !usingToolchainFile;

	if (!usingToolchainFile && m_state.info.generateCompileCommands())
	{
		if (!isDefined["EXPORT_COMPILE_COMMANDS"])
		{
			outList.emplace_back("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON");
		}
	}

	if (crossCompile)
	{
		if (!isDefined["CMAKE_SYSTEM_NAME"])
		{
			std::string systemName = getCmakeSystemName(targetTriple);
			if (!systemName.empty())
				outList.emplace_back(fmt::format("-DCMAKE_SYSTEM_NAME={}", systemName));
		}

		if (!isDefined["CMAKE_SYSTEM_PROCESSOR"])
		{
			outList.emplace_back(fmt::format("-DCMAKE_SYSTEM_PROCESSOR={}", m_state.info.targetArchitectureString()));
		}
	}

	bool needsCMakeProgram = !m_state.environment->isMsvc();
	if (needsCMakeProgram && !isDefined["CMAKE_MAKE_PROGRAM"])
	{
		if (usesNinja())
		{
			const auto& ninja = m_state.toolchain.ninja();
			if (!ninja.empty())
				outList.emplace_back(fmt::format("-DCMAKE_MAKE_PROGRAM={}", getQuotedPath(ninja)));
		}
		else
		{
			const auto& make = m_state.toolchain.make();
			if (!make.empty())
				outList.emplace_back(fmt::format("-DCMAKE_MAKE_PROGRAM={}", getQuotedPath(make)));
		}
	}

	if (!usingToolchainFile && !isDefined["CMAKE_C_COMPILER"])
	{
		const auto& compiler = m_state.toolchain.compilerC().path;
		if (!compiler.empty())
			outList.emplace_back(fmt::format("-DCMAKE_C_COMPILER={}", getQuotedPath(compiler)));
	}

	if (!usingToolchainFile && !isDefined["CMAKE_CXX_COMPILER"])
	{
		const auto& compiler = m_state.toolchain.compilerCpp().path;
		if (!compiler.empty())
			outList.emplace_back(fmt::format("-DCMAKE_CXX_COMPILER={}", getQuotedPath(compiler)));
	}

	if (m_state.environment->isWindowsTarget() && !usingToolchainFile && !isDefined["CMAKE_RC_COMPILER"])
	{
		const auto& compiler = m_state.toolchain.compilerWindowsResource();
		if (!compiler.empty())
			outList.emplace_back(fmt::format("-DCMAKE_RC_COMPILER={}", getQuotedPath(compiler)));
	}

	if (m_state.info.compilerCache() && !usingToolchainFile && !isDefined["CMAKE_CXX_COMPILER_LAUNCHER"])
	{
		auto& ccache = m_state.tools.ccache();
		if (!ccache.empty())
			outList.emplace_back(fmt::format("-DCMAKE_CXX_COMPILER_LAUNCHER={}", getQuotedPath(ccache)));
	}

	if (!usingToolchainFile && !isDefined["CMAKE_BUILD_TYPE"])
	{
		auto buildConfiguration = getCMakeCompatibleBuildConfiguration();
		outList.emplace_back("-DCMAKE_BUILD_TYPE=" + std::move(buildConfiguration));
	}

	if (!usingToolchainFile && !isDefined["CMAKE_LIBRARY_ARCHITECTURE"])
	{
		outList.emplace_back(fmt::format("-DCMAKE_LIBRARY_ARCHITECTURE={}", targetTriple));
	}

	if (!usingToolchainFile && !isDefined["CMAKE_BUILD_WITH_INSTALL_RPATH"])
	{
		outList.emplace_back("-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON");
	}

	if (crossCompile)
	{
		if (!isDefined["CMAKE_LIBRARY_PATH"])
		{
			StringList paths;
			if (!m_state.toolchain.compilerCpp().libDir.empty())
				paths.emplace_back(m_state.toolchain.compilerCpp().libDir);

			if (!m_state.toolchain.compilerC().libDir.empty())
				List::addIfDoesNotExist(paths, std::string(m_state.toolchain.compilerC().libDir));

			if (!paths.empty())
				outList.emplace_back("-DCMAKE_LIBRARY_PATH=" + getQuotedPath(String::join(paths, ';')));
		}

		if (!isDefined["CMAKE_INCLUDE_PATH"])
		{
			StringList paths;
			if (!m_state.toolchain.compilerCpp().includeDir.empty())
				paths.emplace_back(m_state.toolchain.compilerCpp().includeDir);

			if (!m_state.toolchain.compilerC().includeDir.empty())
				List::addIfDoesNotExist(paths, std::string(m_state.toolchain.compilerC().includeDir));

			if (!paths.empty())
				outList.emplace_back("-DCMAKE_INCLUDE_PATH=" + getQuotedPath(String::join(paths, ';')));
		}

		if (!isDefined["CMAKE_FIND_ROOT_PATH_MODE_PROGRAM"])
		{
			outList.emplace_back("-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER");
		}
		if (!isDefined["CMAKE_FIND_ROOT_PATH_MODE_LIBRARY"])
		{
			outList.emplace_back("-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY");
		}
		if (!isDefined["CMAKE_FIND_ROOT_PATH_MODE_INCLUDE"])
		{
			outList.emplace_back("-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY");
		}
		if (!isDefined["CMAKE_FIND_ROOT_PATH_MODE_PACKAGE"])
		{
			outList.emplace_back("-DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY");
		}

		if (m_state.environment->isClang())
		{
			if (!isDefined["CMAKE_C_COMPILER_TARGET"])
			{
				outList.emplace_back(fmt::format("-DCMAKE_C_COMPILER_TARGET={}", targetTriple));
			}
			if (!isDefined["CMAKE_CXX_COMPILER_TARGET"])
			{
				outList.emplace_back(fmt::format("-DCMAKE_CXX_COMPILER_TARGET={}", targetTriple));
			}
		}
	}

#if defined(CHALET_WIN32)
	/*if (!usingToolchainFile && !isDefined["CMAKE_SH"])
	{
		if (Files::which("sh").empty())
			outList.emplace_back("-DCMAKE_SH=\"CMAKE_SH-NOTFOUND\"");
	}*/
#elif defined(CHALET_MACOS)
	if (!usingToolchainFile && m_state.environment->isAppleClang())
	{
		if (!isDefined["CMAKE_OSX_SYSROOT"])
		{
			const auto& osTargetName = m_state.inputs.osTargetName();
			if (!osTargetName.empty())
			{
				auto kAllowedTargets = CompilerCxxAppleClang::getAllowedSDKTargets();
				if (String::equals(kAllowedTargets, osTargetName))
				{
					auto sdkPath = m_state.tools.getApplePlatformSdk(osTargetName);
					if (!sdkPath.empty())
					{
						outList.emplace_back("-DCMAKE_OSX_SYSROOT=" + sdkPath);
					}
				}
			}
		}
		if (!isDefined["CMAKE_OSX_DEPLOYMENT_TARGET"])
		{
			const auto& osTargetVersion = m_state.inputs.osTargetVersion();
			if (!osTargetVersion.empty())
			{
				outList.emplace_back("-DCMAKE_OSX_DEPLOYMENT_TARGET=" + osTargetVersion);
			}
		}
		if (!isDefined["CMAKE_OSX_ARCHITECTURES"])
		{
			if (!m_state.inputs.universalArches().empty())
			{
				auto value = String::join(m_state.inputs.universalArches(), ';');
				outList.emplace_back("-DCMAKE_OSX_ARCHITECTURES=" + std::move(value));
			}
			else
			{
				const auto& targetArch = m_state.info.targetArchitectureString();
				outList.emplace_back("-DCMAKE_OSX_ARCHITECTURES=" + std::move(targetArch));
			}
		}
	}
#endif

	if (usingToolchainFile)
	{
		/*if (isEmscripten && !isDefined["CMAKE_EXECUTABLE_SUFFIX"])
		{
			outList.emplace_back("-DCMAKE_EXECUTABLE_SUFFIX=.html");
		}*/

		if (isEmscripten && !isDefined["CMAKE_TOOLCHAIN_FILE"])
		{
			auto emUpstream = Environment::getString("EMSDK_UPSTREAM_EMSCRIPTEN");
			chalet_assert(!emUpstream.empty(), "'EMSDK_UPSTREAM_EMSCRIPTEN' was not set");

			auto toolchainFile = fmt::format("{}/cmake/Modules/Platform/Emscripten.cmake", emUpstream);
			outList.emplace_back("-DCMAKE_TOOLCHAIN_FILE=" + std::move(toolchainFile));
		}

		if (isEmscripten && !isDefined["CMAKE_CROSSCOMPILING_EMULATOR"])
		{
			auto nodePath = Environment::getString("EMSDK_NODE");
			chalet_assert(!nodePath.empty(), "'EMSDK_NODE' was not set");

			auto versionOutput = Process::runOutput({ nodePath, "--version" });
			if (String::startsWith('v', versionOutput))
				versionOutput = versionOutput.substr(1);

			auto version = Version::fromString(versionOutput);
			u32 versionMajor = version.major();

			StringList nodeArgs{ nodePath };
			if (versionMajor > 0 && versionMajor < 16)
			{
				nodeArgs.emplace_back("--experimental-wasm-bulk-memory");
				nodeArgs.emplace_back("--experimental-wasm-threads");
			}
			outList.emplace_back("-DCMAKE_CROSSCOMPILING_EMULATOR=" + String::join(nodeArgs, ';'));
		}
	}
}

/*****************************************************************************/
std::string CmakeBuilder::getCMakeCompatibleBuildConfiguration() const
{
	std::string ret = m_state.configuration.name();

	if (m_state.configuration.isMinSizeRelease())
	{
		ret = "MinSizeRel";
	}
	else if (m_state.configuration.isReleaseWithDebugInfo())
	{
		ret = "RelWithDebInfo";
	}
	else if (m_state.configuration.isDebuggable())
	{
		// Profile > Debug in CMake
		ret = "Debug";
	}
	else
	{
		// RelHighOpt > Release in CMake
		ret = "Release";
	}

	return ret;
}

/*****************************************************************************/
StringList CmakeBuilder::getBuildCommand() const
{
	auto outputLocation = m_target.targetFolder();
	return getBuildCommand(outputLocation);
}

/*****************************************************************************/
StringList CmakeBuilder::getBuildCommand(const std::string& inOutputLocation) const
{
	auto& cmake = m_state.toolchain.cmake();
	const auto maxJobs = m_state.info.maxJobs();
	const bool isMake = m_state.toolchain.strategy() == StrategyType::Makefile;
	const bool isNinja = usesNinja();

	auto buildLocation = Files::getAbsolutePath(inOutputLocation);
	StringList ret{ getQuotedPath(cmake), "--build", getQuotedPath(buildLocation), "-j", std::to_string(maxJobs) };

	const auto& targets = m_target.targets();
	if (!targets.empty())
	{
		ret.emplace_back("-t");
		for (const auto& name : targets)
		{
			ret.emplace_back(name);
		}
	}

	if (isNinja)
	{
		ret.emplace_back("--");

		if (Output::showCommands())
			ret.emplace_back("-v");

		ret.emplace_back("-k");
		ret.emplace_back(m_state.info.keepGoing() ? "0" : "1");
	}
	else if (isMake)
	{
		ret.emplace_back("--");

		if (m_state.toolchain.makeVersionMajor() >= 4)
			ret.emplace_back("--output-sync=target");

		if (m_state.info.keepGoing())
			ret.emplace_back("--keep-going");

#if defined(CHALET_WIN32)
		if (!m_state.toolchain.makeIsNMake())
		{
			ret.emplace_back("--no-builtin-rules");
			ret.emplace_back("--no-builtin-variables");
			ret.emplace_back("--no-print-directory");
		}
#endif
	}

	// LOG(String::join(ret));

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getCmakeSystemName(const std::string& inTargetTriple) const
{
	// Full-ish list here: https://gitlab.kitware.com/cmake/cmake/-/issues/21489#note_1077167
	// TODO: Android, iOS, etc.

	std::string ret;
	if (String::contains({ "windows", "mingw" }, inTargetTriple))
		ret = "Windows";
	else if (String::contains({ "apple", "darwin" }, inTargetTriple))
		ret = "Darwin";
	else
		ret = "Linux";

	return ret;
}

/*****************************************************************************/
std::string CmakeBuilder::getQuotedPath(const std::string& inPath) const
{
	if (m_quotedPaths)
		return fmt::format("\"{}\"", inPath);
	else
		return inPath;
}

/*****************************************************************************/
bool CmakeBuilder::usesNinja() const
{
	// Note: Some CMake projects might vary between Visual Studio and Ninja generators
	//   The MSBuild strategy doesn't actually care if Cmake projects are built with visual studio since
	//   it just executes cmake as a script, so we'll just use Ninja in that scenario
	//
	auto strategy = m_state.toolchain.strategy();
	if (strategy == StrategyType::Ninja
		|| strategy == StrategyType::MSBuild
		|| strategy == StrategyType::XcodeBuild)
		return true;

	auto& ninjaExec = m_state.toolchain.ninja();
	return !ninjaExec.empty() && Files::pathExists(ninjaExec);
}

/*****************************************************************************/
const std::string& CmakeBuilder::outputLocation() const
{
	return m_target.targetFolder();
}

}
