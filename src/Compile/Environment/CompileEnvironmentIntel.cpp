/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentIntel.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Environment/IntelEnvironmentScript.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentIntel::CompileEnvironmentIntel(const ToolchainType inType, BuildState& inState) :
	CompileEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
CompileEnvironmentIntel::~CompileEnvironmentIntel() = default;

/*****************************************************************************/
StringList CompileEnvironmentIntel::getVersionCommand(const std::string& inExecutable) const
{
	if (m_type == ToolchainType::IntelLLVM)
		return { inExecutable, "-target", m_state.info.targetArchitectureTriple(), "-v" };
	else
		return { inExecutable, "-V" };
}

/*****************************************************************************/
std::string CompileEnvironmentIntel::getFullCxxCompilerString(const std::string& inVersion) const
{
	if (m_type == ToolchainType::IntelLLVM)
		return fmt::format("Intel{} oneAPI DPC++/C++ version {}", Unicode::registered(), inVersion);
	else
		return fmt::format("Intel{} 64 Compiler Classic version {}", Unicode::registered(), inVersion);
}

/*****************************************************************************/
bool CompileEnvironmentIntel::verifyCompilerExecutable(const std::string& inCompilerExec)
{
#if defined(CHALET_WIN32)
	if (m_type == ToolchainType::IntelClassic)
		return true;
#endif

	return CompileEnvironmentGNU::verifyCompilerExecutable(inCompilerExec);
}

/*****************************************************************************/
ToolchainType CompileEnvironmentIntel::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	if (m_type == ToolchainType::IntelLLVM)
	{
		auto llvmType = CompileEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
		if (llvmType != ToolchainType::LLVM)
			return llvmType;

		const bool intelClang = String::contains({ "__INTEL_LLVM_COMPILER", "__INTEL_CLANG_COMPILER" }, inMacros);
		if (intelClang)
			return ToolchainType::IntelLLVM;
	}
	else
	{
#if defined(CHALET_WIN32)
		return ToolchainType::IntelClassic;
#else
		auto gccType = CompileEnvironmentGNU::getToolchainTypeFromMacros(inMacros);
		if (gccType != ToolchainType::GNU)
			return gccType;

		const bool intelGcc = String::contains({ "__INTEL_COMPILER", "__INTEL_COMPILER_BUILD_DATE" }, inMacros);
		if (intelGcc)
			return ToolchainType::IntelClassic;
#endif
	}

	return ToolchainType::Unknown;
}

/*****************************************************************************/
std::vector<CompilerPathStructure> CompileEnvironmentIntel::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret = CompileEnvironmentGNU::getValidCompilerPaths();

	if (m_type == ToolchainType::IntelLLVM)
	{
#if defined(CHALET_WIN32)
		ret.push_back({ "/bin/intel64", "/compiler/lib/intel64_win", "/compiler/include" });
		ret.push_back({ "/bin/intel64_ia32", "/compiler/lib/ia32_win", "/compiler/include" });
#else
		// TODO: Linux
#endif
	}
	else
	{
		ret.push_back({ "/bin/intel64", "/compiler/lib", "/compiler/include" });
		// TODO: Linux
	}

	return ret;
}

/*****************************************************************************/
bool CompileEnvironmentIntel::validateArchitectureFromInput()
{
	std::string target = m_state.inputs.targetArchitecture();
	if (target.empty())
	{
		target = m_state.inputs.hostArchitecture();

		m_state.inputs.setTargetArchitecture(target);
		m_state.info.setTargetArchitecture(m_state.inputs.targetArchitecture());
	}

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentIntel::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	Timer timer;

	m_config = std::make_unique<IntelEnvironmentScript>(m_state.inputs);

	m_config->setEnvVarsFileBefore(m_state.cache.getHashPath(fmt::format("{}_original.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileAfter(m_state.cache.getHashPath(fmt::format("{}_all.env", this->identifier()), CacheType::Local));
	m_config->setEnvVarsFileDelta(getVarsPath("0"));

	m_ouptuttedDescription = true;

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
		vsConfig.varsFileOriginal = m_state.cache.getHashPath("msvc_original.env", CacheType::Local);
		vsConfig.varsFileMsvc = m_state.cache.getHashPath("msvc_all.env", CacheType::Local);
		vsConfig.varsFileMsvcDelta = m_state.cache.getHashPath(fmt::format("msvc_{}_delta.env", vsConfig.varsAllArch), CacheType::Local);
		vsConfig.varsAllArchOptions = m_state.inputs.archOptions();
		vsConfig.isPreset = isPresetFromInput;

		if (!CompileEnvironmentVisualStudio::makeEnvironment(vsConfig, std::string()))
			return false;

		variables.clear();
		Environment::readEnvFileToDictionary(vsConfig.varsFileMsvcDelta, variables);

		CompileEnvironmentVisualStudio::populateVariables(vsConfig, variables);

		Diagnostic::printDone(vsTimer.asString());
	}*/
#endif

	return true;
}

/*****************************************************************************/
std::string CompileEnvironmentIntel::makeToolchainName(const std::string& inArch) const
{
	std::string ret;
	if (m_type == ToolchainType::IntelLLVM)
	{
		ret = fmt::format("{}-intel-llvm", inArch);

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
	else
	{
		ret = fmt::format("{}-intel-classic", inArch);
	}
	return ret;
}

/*****************************************************************************/
bool CompileEnvironmentIntel::readArchitectureTripleFromCompiler()
{
	if (m_type == ToolchainType::IntelLLVM)
		return CompileEnvironmentLLVM::readArchitectureTripleFromCompiler();
#if !defined(CHALET_WIN32)
	else
		return CompileEnvironmentGNU::readArchitectureTripleFromCompiler();
#endif

	return true;
}

/*****************************************************************************/
void CompileEnvironmentIntel::parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const
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
bool CompileEnvironmentIntel::populateSupportedFlags(const std::string& inExecutable)
{
	if (m_type == ToolchainType::IntelLLVM)
	{
		return CompileEnvironmentLLVM::populateSupportedFlags(inExecutable);
	}
	else
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
		CompileEnvironmentGNU::parseSupportedFlagsFromHelpList(cmd);

		return true;
	}
}
}
