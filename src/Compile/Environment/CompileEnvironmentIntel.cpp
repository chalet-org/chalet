/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentIntel.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentIntel::CompileEnvironmentIntel(const CommandLineInputs& inInputs, BuildState& inState, const ToolchainType inType) :
	CompileEnvironmentLLVM(inInputs, inState),
	kVarsId("intel"),
	m_type(inType)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentIntel::type() const noexcept
{
	return m_type;
}

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
bool CompileEnvironmentIntel::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	Timer timer;

	m_varsFileOriginal = m_state.cache.getHashPath(fmt::format("{}_original.env", kVarsId), CacheType::Local);
	m_varsFileIntel = m_state.cache.getHashPath(fmt::format("{}_all.env", kVarsId), CacheType::Local);
	m_varsFileIntelDelta = getVarsPath(kVarsId);
	m_path = Environment::getPath();

	bool isPresetFromInput = m_inputs.isToolchainPreset();

	bool deltaExists = Commands::pathExists(m_varsFileIntelDelta);
	if (!deltaExists)
	{
		Diagnostic::infoEllipsis("Creating Intel{} C/C++ Environment Cache", Unicode::registered());

#if defined(CHALET_WIN32)
		auto oneApiRoot = Environment::get("ONEAPI_ROOT");
		m_intelSetVars = fmt::format("{}/setvars.bat", oneApiRoot);
#else
		const auto& home = m_inputs.homeDirectory();
		m_intelSetVars = fmt::format("{}/intel/oneapi/setvars.sh", home);
#endif
		if (!Commands::pathExists(m_intelSetVars))
		{
#if !defined(CHALET_WIN32)
			m_intelSetVars = "/opt/intel/oneapi/setvars.sh";
			if (!Commands::pathExists(m_intelSetVars))
#endif
			{
				Diagnostic::error("No suitable Intel C++ compiler installation found. Pleas install the Intel oneAPI Toolkit before continuing.");
				return false;
			}
		}

		// Read the current environment and save it to a file
		if (!saveOriginalEnvironment(m_varsFileOriginal))
		{
			Diagnostic::error("Intel Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		if (!saveIntelEnvironment())
		{
			Diagnostic::error("Intel Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		createEnvironmentDelta(m_varsFileOriginal, m_varsFileIntel, m_varsFileIntelDelta, [this](std::string& line) {
			if (String::startsWith({ "PATH=", "Path=" }, line))
			{
				String::replaceAll(line, m_path, "");
			}
		});
	}
	else
	{
		Diagnostic::infoEllipsis("Reading Intel{} C/C++ Environment Cache", Unicode::registered());
	}

	// Read delta to cache
	cacheEnvironmentDelta(m_varsFileIntelDelta);

	StringList pathSearch{ "Path", "PATH" };
	for (auto& [name, var] : m_variables)
	{
		if (String::equals(pathSearch, name))
		{
			// if (String::contains(inject, m_path))
			// {
			// 	String::replaceAll(m_path, inject, var);
			// 	Environment::set(name.c_str(), m_path);
			// }
			// else
			{
				Environment::set(name.c_str(), m_path + ":" + var);
			}
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (isPresetFromInput)
	{
#if defined(CHALET_MACOS)
		std::string name = fmt::format("{}-apple-darwin-icc", m_inputs.targetArchitecture());
#else
		std::string name;
		if (m_type == ToolchainType::IntelLLVM)
			name = fmt::format("{}-pc-windows-icx", m_inputs.targetArchitecture());
		else
			name = fmt::format("{}-pc-windows-icc", m_inputs.targetArchitecture());

#endif
		m_inputs.setToolchainPreferenceName(std::move(name));

		auto old = m_varsFileIntelDelta;
		m_varsFileIntelDelta = getVarsPath(kVarsId);
		if (m_varsFileIntelDelta != old)
		{
			Commands::copyRename(old, m_varsFileIntelDelta, true);
		}
	}

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileIntelDelta));

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentIntel::validateArchitectureFromInput()
{
	if (m_inputs.targetArchitecture().empty())
	{
		// Try to get the architecture from the name
		std::string target;
		const auto& preferenceName = m_inputs.toolchainPreferenceName();
		auto regexResult = RegexPatterns::matchesTargetArchitectureWithResult(preferenceName);
		if (!regexResult.empty())
		{
			target = regexResult;
		}
		else
		{
			target = m_inputs.hostArchitecture();
		}

		m_inputs.setTargetArchitecture(target);
		m_state.info.setTargetArchitecture(m_inputs.targetArchitecture());
	}

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentIntel::makeArchitectureAdjustments()
{
	if (m_type == ToolchainType::IntelLLVM)
	{
		return CompileEnvironmentLLVM::makeArchitectureAdjustments();
	}

#if defined(CHALET_MACOS)
	auto arch = m_inputs.hostArchitecture();
	m_state.info.setTargetArchitecture(fmt::format("{}-intel-darwin", arch));
#endif

	return true;
}

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
bool CompileEnvironmentIntel::saveIntelEnvironment() const
{
#if defined(CHALET_WIN32)
	StringList cmd{ m_intelSetVars };

	const auto inArch = m_state.info.targetArchitecture();
	if (inArch != Arch::Cpu::X64 && inArch != Arch::Cpu::X86)
	{
		auto setVarsFile = String::getPathFilename(m_intelSetVars);
		Diagnostic::error("Requested arch '{}' is not supported by {}", m_inputs.targetArchitecture(), setVarsFile);
		return false;
	}

	std::string arch;
	if (inArch == Arch::Cpu::X86)
		arch = "ia32";
	else
		arch = "intel64";

	cmd.emplace_back(std::move(arch));

	const auto vsVersion = m_inputs.visualStudioVersion();
	if (vsVersion == VisualStudioVersion::VisualStudio2022)
		cmd.emplace_back("vs2022");
	if (vsVersion == VisualStudioVersion::VisualStudio2019)
		cmd.emplace_back("vs2019");
	if (vsVersion == VisualStudioVersion::VisualStudio2017)
		cmd.emplace_back("vs2019");

	cmd.emplace_back(">");
	cmd.emplace_back("nul");
	cmd.emplace_back("&&");
	cmd.emplace_back("SET");
	cmd.emplace_back(">");
	cmd.push_back(m_varsFileIntel);
#else
	StringList bashCmd;
	bashCmd.emplace_back("source");
	bashCmd.push_back(m_intelSetVars);
	bashCmd.emplace_back("--force");
	bashCmd.emplace_back(">");
	bashCmd.emplace_back("/dev/null");
	bashCmd.emplace_back("&&");
	bashCmd.emplace_back("printenv");
	bashCmd.emplace_back(">");
	bashCmd.push_back(m_varsFileIntel);
	auto bashCmdString = String::join(bashCmd);

	auto sh = Commands::which("sh");

	StringList cmd;
	cmd.push_back(sh);
	cmd.emplace_back("-c");
	cmd.emplace_back(fmt::format("'{}'", bashCmdString));

#endif
	auto outCmd = String::join(cmd);
	bool result = std::system(outCmd.c_str()) == EXIT_SUCCESS;

	return result;
}
}
