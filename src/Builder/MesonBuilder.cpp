/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/MesonBuilder.hpp"

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
#include "State/Target/MesonTarget.hpp"
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
MesonBuilder::MesonBuilder(const BuildState& inState, const MesonTarget& inTarget, const bool inQuotedPaths) :
	m_state(inState),
	m_target(inTarget),
	m_quotedPaths(inQuotedPaths)
{
	m_mesonVersionMajorMinor = m_state.toolchain.mesonVersionMajor();
	m_mesonVersionMajorMinor *= 100;
	m_mesonVersionMajorMinor += m_state.toolchain.mesonVersionMinor();
}

/*****************************************************************************/
std::string MesonBuilder::getLocation() const
{
	const auto& rawLocation = m_target.location();
	auto ret = Files::getAbsolutePath(rawLocation);
	Path::toUnix(ret);

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getBuildFile(const bool inForce) const
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
		ret = fmt::format("{}/meson.build", location);
	}

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getCacheFile() const
{
	std::string ret;

	auto location = m_target.targetFolder();
	ret = fmt::format("{}/build.ninja", location);

	return ret;
}

/*****************************************************************************/
bool MesonBuilder::dependencyHasUpdated() const
{
	if (m_target.hashChanged())
	{
		Files::removeRecursively(m_target.targetFolder());
		return true;
	}

	return false;
}

/*****************************************************************************/
bool MesonBuilder::run()
{
	Timer buildTimer;

	const auto& name = m_target.name();

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

	auto& buildDir = outputLocation();

	auto& sourceCache = m_state.cache.file().sources();
	auto outputHash = Hash::string(buildDir);
	bool lastBuildFailed = sourceCache.dataCacheValueIsFalse(outputHash);
	bool dependencyUpdated = dependencyHasUpdated();

	bool outDirectoryDoesNotExist = !Files::pathExists(buildDir);
	bool recheckMeson = m_target.recheck() || lastBuildFailed || dependencyUpdated;

	if (outDirectoryDoesNotExist || recheckMeson)
	{
		if (outDirectoryDoesNotExist)
			Files::makeDirectory(buildDir);

		bool runMesonSetup = outDirectoryDoesNotExist || lastBuildFailed || dependencyUpdated;

		// if (isNinja)
		// {
		// 	const auto& color = Output::getAnsiStyle(Output::theme().build);
		// 	Environment::set(kNinjaStatus, fmt::format("   [%f/%t] {}", color));
		// }

		StringList command;
		if (runMesonSetup)
		{
			if (!createNativeFile())
				return onRunFailure();

			command = getSetupCommand();

			if (!Process::run(command))
				return onRunFailure();
		}

		command = getBuildCommand(buildDir);

		// bool result = Process::runNinjaBuild(command);
		bool result = Process::run(command);
		if (result && m_target.install())
		{
			command = getInstallCommand(buildDir);
			result = Process::run(command);
		}

		sourceCache.addDataCache(outputHash, result);
		if (!result)
			return onRunFailure(false);

		if (isNinja)
			Environment::set(kNinjaStatus, oldNinjaStatus);
	}

	//
	Output::msgTargetUpToDate(name, &buildTimer);

	return true;
}

/*****************************************************************************/
bool MesonBuilder::createNativeFile() const
{
	auto nativeFile = getNativeFileOutputPath();

	const auto& toolchain = m_state.toolchain;
	const auto& ninja = toolchain.ninja();
	const auto& archTriple = m_state.info.targetArchitectureTriple();

	auto compilerC = toolchain.compilerC().path;
	auto compilerCpp = toolchain.compilerCpp().path;
#if defined(CHALET_MACOS)
	Path::stripXcodeToolchain(compilerC);
	Path::stripXcodeToolchain(compilerCpp);
#endif
	if (m_state.info.compilerCache())
	{
		auto& ccache = m_state.tools.ccache();
		compilerC = fmt::format("['{}', '{}']", ccache, compilerC);
		compilerCpp = fmt::format("['{}', '{}']", ccache, compilerCpp);
	}
	else
	{
		compilerC = fmt::format("'{}'", compilerC);
		compilerCpp = fmt::format("'{}'", compilerCpp);
	}

	auto arch = m_state.info.targetArchitectureString();
	auto cpuFamily = getCpuFamily();

	std::string optionsHeading = m_mesonVersionMajorMinor > 56 ? "built-in options" : "properties";

	std::string targetArg;
	if (m_state.environment->isClang())
	{
		targetArg = fmt::format("'--target={}'", archTriple);
	}

	std::string otherBinaries;
	std::string otherProperties;

#if defined(CHALET_MACOS)
	otherBinaries = fmt::format(R"ini(
objc = {compilerC}
objcpp = {compilerCpp})ini",
		FMT_ARG(compilerC),
		FMT_ARG(compilerCpp));

	otherProperties = fmt::format(R"ini(
objc_args = [{targetArg}]
objcpp_args = [{targetArg}]
objc_link_args = [{targetArg}]
objcpp_link_args = [{targetArg}])ini",
		FMT_ARG(targetArg));
#endif

	auto contents = fmt::format(R"ini([binaries]
ninja = '{ninja}'
c = {compilerC}
cpp = {compilerCpp}{otherBinaries}

[{optionsHeading}]
c_args = [{targetArg}]
cpp_args = [{targetArg}]
c_link_args = [{targetArg}]
cpp_link_args = [{targetArg}]{otherProperties}

[target_machine]
cpu_family = '{cpuFamily}'
cpu = '{arch}'
)ini",
		FMT_ARG(ninja),
		FMT_ARG(compilerC),
		FMT_ARG(compilerCpp),
		FMT_ARG(otherBinaries),
		FMT_ARG(otherProperties),
		FMT_ARG(optionsHeading),
		FMT_ARG(targetArg),
		FMT_ARG(cpuFamily),
		FMT_ARG(arch));

	// #if defined(CHALET_WIN32)
	// 				String::replaceAll(contents, "/", "\\\\");
	// #endif

	if (!Files::createFileWithContents(nativeFile, contents))
	{
		Diagnostic::error("Error creating toolchain file for Meson: {}", nativeFile);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string MesonBuilder::getBackend() const
{
	const bool isNinja = usesNinja();

	std::string ret;
	if (isNinja)
	{
		ret = "ninja";
	}
	else
	{
		ret = "none";
	}

	// vs,vs2017,vs2019,vs2022,xcode

	return ret;
}

/*****************************************************************************/
StringList MesonBuilder::getSetupCommand()
{
	auto location = getLocation();
	auto buildFile = getBuildFile();

	return getSetupCommand(location, buildFile);
}

/*****************************************************************************/
StringList MesonBuilder::getSetupCommand(const std::string& inLocation, const std::string& inBuildFile) const
{
	auto& meson = m_state.toolchain.meson();
	auto buildDir = Files::getCanonicalPath(outputLocation());
	auto optimization = getMesonCompatibleOptimizationFlag();

	auto backend = getBackend();
	chalet_assert(!backend.empty(), "Meson Backend is empty");

	auto nativeFile = getNativeFileOutputPath();

	StringList ret{
		getQuotedPath(meson),
		"setup",
		"--backend",
		backend,
		"--native-file",
		getQuotedPath(Files::getCanonicalPath(nativeFile)),
		"--optimization",
		optimization,
	};
	if (m_state.configuration.debugSymbols())
		ret.emplace_back("--debug");
	else if (!m_state.environment->isMsvc())
		ret.emplace_back("--strip");

	auto& defines = m_target.defines();

	for (auto& define : defines)
	{
		ret.emplace_back("-D");
		ret.emplace_back(define);
	}

	if (Output::showCommands())
		ret.emplace_back("--errorlogs");

	ret.emplace_back(getQuotedPath(buildDir));

	ret.emplace_back(getQuotedPath(inLocation));

	UNUSED(inBuildFile);
	// if (!inBuildFile.empty())
	// {
	// 	ret.emplace_back(getQuotedPath(inBuildFile));
	// }
	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getMesonCompatibleBuildConfiguration() const
{
	std::string ret;
	if (m_state.configuration.isMinSizeRelease())
	{
		ret = "minsize";
	}
	else if (m_state.configuration.isReleaseWithDebugInfo())
	{
		ret = "debugoptimized";
	}
	else if (m_state.configuration.isDebuggable())
	{
		// Profile > debug in Meson
		ret = "debug";
	}
	else
	{
		// RelHighOpt > release in Meson
		ret = "release";
	}

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getMesonCompatibleOptimizationFlag() const
{
	// plain,0,g,1,2,3,s
	auto optimizationLevel = m_state.configuration.optimizationLevel();
	switch (optimizationLevel)
	{
		case OptimizationLevel::None:
			return "0";
		case OptimizationLevel::L1:
			return "1";
		case OptimizationLevel::L2:
			return "2";
		case OptimizationLevel::L3:
		case OptimizationLevel::Fast:
			return "3";
		case OptimizationLevel::Debug:
			return "g";
		case OptimizationLevel::Size:
			return "s";
		case OptimizationLevel::CompilerDefault:
		default:
			return "plain";
	}
}

/*****************************************************************************/
StringList MesonBuilder::getBuildCommand() const
{
	auto outputLocation = m_target.targetFolder();
	return getBuildCommand(outputLocation);
}

/*****************************************************************************/
StringList MesonBuilder::getBuildCommand(const std::string& inOutputLocation) const
{
	auto& meson = m_state.toolchain.meson();
	const auto maxJobs = m_state.info.maxJobs();
	const bool isNinja = usesNinja();

	auto buildLocation = Files::getAbsolutePath(inOutputLocation);
	StringList ret{
		getQuotedPath(meson),
		"compile",
		"-C",
		getQuotedPath(buildLocation),
		"--jobs",
		std::to_string(maxJobs),
	};

	if (isNinja)
	{
		std::string ninjaArgs{ "--ninja-args=" };

		if (Output::showCommands())
			ninjaArgs += "-v,";

		ninjaArgs += "-k,";
		ninjaArgs += (m_state.info.keepGoing() ? "0" : "1");

		ret.emplace_back(std::move(ninjaArgs));
	}

	const auto& targets = m_target.targets();
	if (!targets.empty())
	{
		for (const auto& name : targets)
			ret.emplace_back(name);
	}

	// LOG(String::join(ret));

	return ret;
}

/*****************************************************************************/
StringList MesonBuilder::getInstallCommand() const
{
	auto outputLocation = m_target.targetFolder();
	return getInstallCommand(outputLocation);
}

/*****************************************************************************/
StringList MesonBuilder::getInstallCommand(const std::string& inOutputLocation) const
{
	auto& meson = m_state.toolchain.meson();

	auto buildLocation = Files::getAbsolutePath(inOutputLocation);

	StringList ret{
		getQuotedPath(meson),
		"install",
		"-C",
		getQuotedPath(buildLocation),
		"--destdir",
		getQuotedPath(fmt::format("{}/install", buildLocation)),
	};

	// LOG(String::join(ret));

	return ret;
}
/*****************************************************************************/
std::string MesonBuilder::getNativeFileOutputPath() const
{
	auto filename = fmt::format("meson_{}.ini", m_state.info.targetArchitectureTriple());

	if (m_state.inputs.route().isExport())
	{
		auto kind = m_state.inputs.exportKind();
		if (kind == ExportKind::VisualStudioSolution)
		{
			auto& outputDirectory = m_state.paths.outputDirectory();
			return fmt::format("{}/.vssolution/meson/{}", outputDirectory, filename);
		}
		else if (kind == ExportKind::CodeBlocks)
		{
			auto& outputDirectory = m_state.paths.outputDirectory();
			return fmt::format("{}/.codeblocks/meson/{}", outputDirectory, filename);
		}
		else if (kind == ExportKind::Xcode)
		{
			auto& outputDirectory = m_state.paths.outputDirectory();
			return fmt::format("{}/.xcode/meson/{}", outputDirectory, filename);
		}
	}
	return fmt::format("{}/{}", outputLocation(), filename);
}

/*****************************************************************************/
std::string MesonBuilder::getCpuFamily() const
{
	auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::ARM:
		case Arch::Cpu::ARMHF:
			return "arm";
		case Arch::Cpu::ARM64:
			return "aarch64";
		case Arch::Cpu::WASM32:
			return "wasm32";
		case Arch::Cpu::X86:
			return "x86";
		case Arch::Cpu::X64:
		default:
			return "x86_64";
	}
}

/*****************************************************************************/
std::string MesonBuilder::getQuotedPath(const std::string& inPath) const
{
	if (m_quotedPaths)
		return fmt::format("\"{}\"", inPath);
	else
		return inPath;
}

/*****************************************************************************/
bool MesonBuilder::usesNinja() const
{
	// Ignore the build strategy - Meson only supports ninja
	auto& ninjaExec = m_state.toolchain.ninja();
	if (ninjaExec.empty() || !Files::pathExists(ninjaExec))
		return false;

	return true;
}

/*****************************************************************************/
const std::string& MesonBuilder::outputLocation() const
{
	return m_target.targetFolder();
}
}
