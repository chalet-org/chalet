/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentIntel.hpp"

#include "BuildEnvironment/Script/IntelEnvironmentScript.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentIntel::BuildEnvironmentIntel(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
BuildEnvironmentIntel::~BuildEnvironmentIntel() = default;

/*****************************************************************************/
bool BuildEnvironmentIntel::supportsCppModules() const
{
	auto& inputFile = m_state.inputs.inputFile();
	auto& compiler = m_state.toolchain.compilerCpp();
	u32 versionMajorMinor = compiler.versionMajorMinor;
	if (versionMajorMinor < 202310)
	{
		Diagnostic::error("{}: C++ modules are only supported with Clang versions >= 15.0.0 (found {})", inputFile, compiler.version);
		return false;
	}
	return true;
}

/*****************************************************************************/
std::string BuildEnvironmentIntel::getPrecompiledHeaderExtension() const
{
	// if (isIntelClassic())
	// 	return ".pchi";

	return BuildEnvironmentLLVM::getPrecompiledHeaderExtension();
}

/*****************************************************************************/
StringList BuildEnvironmentIntel::getVersionCommand(const std::string& inExecutable) const
{
	if (m_type == ToolchainType::IntelLLVM)
		return { inExecutable, "-target", m_state.info.targetArchitectureTriple(), "-v" };
	else if (m_type == ToolchainType::IntelClassic)
		return { inExecutable, "-V" };
	else
		return StringList{};
}

/*****************************************************************************/
std::string BuildEnvironmentIntel::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);

	if (m_type == ToolchainType::IntelLLVM)
		return fmt::format("Intel{} oneAPI DPC++/C++ version {}", Unicode::registered(), inVersion);
	else if (m_type == ToolchainType::IntelClassic)
		return fmt::format("Intel{} 64 Compiler Classic version {}", Unicode::registered(), inVersion);
	else
		return std::string();
}

/*****************************************************************************/
bool BuildEnvironmentIntel::verifyCompilerExecutable(const std::string& inCompilerExec)
{
#if defined(CHALET_WIN32)
	if (m_type == ToolchainType::IntelClassic)
		return true;
#endif

	return BuildEnvironmentGNU::verifyCompilerExecutable(inCompilerExec);
}

/*****************************************************************************/
ToolchainType BuildEnvironmentIntel::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	if (m_type == ToolchainType::IntelLLVM)
	{
		auto llvmType = BuildEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
		if (llvmType != ToolchainType::LLVM)
			return llvmType;

		const bool intelClang = String::contains(StringList{ "__INTEL_LLVM_COMPILER", "__INTEL_CLANG_COMPILER" }, inMacros);
		if (intelClang)
			return ToolchainType::IntelLLVM;
	}
	else if (m_type == ToolchainType::IntelClassic)
	{
#if defined(CHALET_WIN32)
		return ToolchainType::IntelClassic;
#else
		auto gccType = BuildEnvironmentGNU::getToolchainTypeFromMacros(inMacros);
		if (gccType != ToolchainType::GNU)
			return gccType;

		const bool intelGcc = String::contains(StringList{ "__INTEL_COMPILER", "__INTEL_COMPILER_BUILD_DATE" }, inMacros);
		if (intelGcc)
			return ToolchainType::IntelClassic;
#endif
	}

	return ToolchainType::Unknown;
}

/*****************************************************************************/
std::vector<CompilerPathStructure> BuildEnvironmentIntel::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret = BuildEnvironmentGNU::getValidCompilerPaths();

	// TODO: Linux
	auto arch = m_state.info.targetArchitecture();

	if (m_type == ToolchainType::IntelLLVM)
	{
#if defined(CHALET_WIN32)
		if (arch == Arch::Cpu::X64)
		{
			ret.push_back({ "/bin/intel64", "/compiler/lib/intel64_win", "/compiler/include" });
			ret.push_back({ "/bin-llvm", "/compiler/lib/intel64_win", "/compiler/include" });
		}
		else if (arch == Arch::Cpu::X86)
		{
			ret.push_back({ "/bin/intel64_ia32", "/compiler/lib/ia32_win", "/compiler/include" });
			ret.push_back({ "/bin-llvm", "/compiler/lib/ia32_win", "/compiler/include" });
		}
#else
#endif
	}
	else if (m_type == ToolchainType::IntelClassic)
	{
		if (arch == Arch::Cpu::X64)
		{
			ret.push_back({ "/bin/intel64", "/compiler/lib", "/compiler/include" });
		}
	}

	return ret;
}

/*****************************************************************************/
bool BuildEnvironmentIntel::validateArchitectureFromInput()
{
	m_state.info.setTargetArchitecture(m_state.inputs.getResolvedTargetArchitecture());

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentIntel::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	Timer timer;

	m_config = std::make_unique<IntelEnvironmentScript>(m_state.inputs);

	m_config->setEnvVarsFileBefore(getCachePath("original.env"));
	m_config->setEnvVarsFileAfter(getCachePath("all.env"));
	m_config->setEnvVarsFileDelta(getVarsPath());

	if (m_config->envVarsFileDeltaExists())
		Diagnostic::infoEllipsis("Reading Intel{} C/C++ Environment Cache", Unicode::registered());
	else
		Diagnostic::infoEllipsis("Creating Intel{} C/C++ Environment Cache", Unicode::registered());

	if (!m_config->makeEnvironment(m_state))
		return false;

	m_config->readEnvironmentVariablesFromDeltaFile();

	if (m_config->isPreset())
		m_state.inputs.setToolchainPreferenceName(makeToolchainName(m_state.info.targetArchitectureString()));

	m_state.cache.file().addExtraHash(String::getPathFilename(m_config->envVarsFileDelta()));

	m_config.reset(); // No longer needed

	Diagnostic::printDone(timer.asString());

#if defined(CHALET_WIN32)
	/*if (m_type == ToolchainType::IntelClassic)
	{
		Timer vsTimer;

		VisualStudioEnvironmentConfig vsConfig;
		vsConfig.inVersion = m_state.inputs.visualStudioVersion();

		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
			vsConfig.varsAllArch += "x64";
		if (m_state.info.targetArchitecture() == Arch::Cpu::X86)
			vsConfig.varsAllArch += "x86";

		vsConfig.varsAllArchOptions = m_state.inputs.archOptions();
		vsConfig.varsFileOriginal = m_state.cache.getHashPath("msvc_original.env");
		vsConfig.varsFileMsvc = m_state.cache.getHashPath("msvc_all.env");
		vsConfig.varsFileMsvcDelta = m_state.cache.getHashPath(fmt::format("msvc_{}_delta.env", vsConfig.varsAllArch));
		vsConfig.varsAllArchOptions = m_state.inputs.archOptions();
		vsConfig.isPreset = isPresetFromInput;

		if (!BuildEnvironmentVisualStudio::makeEnvironment(vsConfig, std::string()))
			return false;

		variables.clear();
		Environment::readEnvFileToDictionary(vsConfig.varsFileMsvcDelta, variables);

		BuildEnvironmentVisualStudio::populateVariables(vsConfig, variables);

		Diagnostic::printDone(vsTimer.asString());
	}*/
#endif

	return true;
}

/*****************************************************************************/
std::string BuildEnvironmentIntel::makeToolchainName(const std::string& inArch) const
{
	std::string ret;
	if (m_type == ToolchainType::IntelLLVM)
	{
		// ret = fmt::format("{}-intel-llvm", inArch);
		UNUSED(inArch);
		ret = "intel-llvm";

#if defined(CHALET_WIN32)
		const auto vsVersion = m_state.inputs.visualStudioVersion();
		if (vsVersion == VisualStudioVersion::VisualStudio2022)
			ret += "-vs-2022";
		if (vsVersion == VisualStudioVersion::VisualStudio2019)
			ret += "-vs-2019";
		if (vsVersion == VisualStudioVersion::VisualStudio2017)
			ret += "-vs-2017";
#endif
	}
	else if (m_type == ToolchainType::IntelClassic)
	{
		ret = fmt::format("{}-intel-classic", inArch);
	}

	return ret;
}

/*****************************************************************************/
bool BuildEnvironmentIntel::readArchitectureTripleFromCompiler()
{
	if (m_type == ToolchainType::IntelLLVM)
		return BuildEnvironmentLLVM::readArchitectureTripleFromCompiler();
#if !defined(CHALET_WIN32)
	else if (m_type == ToolchainType::IntelClassic)
		return BuildEnvironmentGNU::readArchitectureTripleFromCompiler();
#endif

	return true;
}

/*****************************************************************************/
void BuildEnvironmentIntel::parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const
{
	if (!String::contains("Intel", inLine))
		return;

	auto start = inLine.find("Version ");
	if (start != std::string::npos)
	{
		start += 8;
		auto end = inLine.find(' ', start);
		outVersion = inLine.substr(start, end - start);
	}
	else
	{
		start = inLine.find("Compiler ");
		if (start != std::string::npos)
		{
			start += 9;
			auto end = inLine.find(' ', start);
			outVersion = inLine.substr(start, end - start);
		}
	}
}

/*****************************************************************************/
bool BuildEnvironmentIntel::populateSupportedFlags(const std::string& inExecutable)
{
	if (m_type == ToolchainType::IntelClassic)
	{
		StringList categories{
			"codegen",
			"compatibility",
			"advanced",
			"component",
			"data",
			"diagnostics",
			"float",
			"inline",
			"ipo",
			"language",
			"link",
			"misc",
			"opt",
			"output",
			"pgo",
			"preproc",
			"reports",
			"openmp",
		};
		StringList cmd{ inExecutable, "-Q" };
		std::string help{ "--help" };
		for (auto& category : categories)
		{
			cmd.push_back(help);
			cmd.emplace_back(category);
		}
		BuildEnvironmentGNU::parseSupportedFlagsFromHelpList(cmd);

		return true;
	}
	else
	{
		return BuildEnvironmentLLVM::populateSupportedFlags(inExecutable);
	}
}

/*****************************************************************************/
std::string BuildEnvironmentIntel::getPrecompiledHeaderSourceFile(const std::string& inSource) const
{
	if (m_type == ToolchainType::IntelClassic)
	{
		const auto& cxxExt = m_state.paths.cxxExtension();
		if (cxxExt.empty())
			return std::string();

		return fmt::format("{}/{}.{}", m_state.paths.objDir(), m_state.paths.getNormalizedOutputPath(inSource), cxxExt);
	}
	else
	{
		return IBuildEnvironment::getPrecompiledHeaderSourceFile(inSource);
	}
}
}
