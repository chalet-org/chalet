/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentLLVM.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentLLVM::BuildEnvironmentLLVM(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentGNU(inType, inState)
{
}

/*****************************************************************************/
std::string BuildEnvironmentLLVM::getStaticLibraryExtension() const
{
	if (isWindowsClang())
		return ".lib";
	else
		return ".a";
}

/*****************************************************************************/
std::string BuildEnvironmentLLVM::getPrecompiledHeaderExtension() const
{
	return ".pch";
}

/*****************************************************************************/
std::string BuildEnvironmentLLVM::getCompilerAliasForVisualStudio() const
{
	return "clang";
}

/*****************************************************************************/
StringList BuildEnvironmentLLVM::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable, "-target", m_state.info.targetArchitectureTriple(), "-v" };
}

/*****************************************************************************/
std::string BuildEnvironmentLLVM::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	if (m_state.toolchain.treatAs() == CustomToolchainTreatAs::LLVM)
	{
		auto name = String::getPathBaseName(inPath);
		if (!name.empty())
		{
			String::capitalize(name);
			String::replaceAll(name, '+', "");
		}

		return fmt::format("{} version {} (Based on LLVM Clang)", name, inVersion);
	}
	else if (m_type == ToolchainType::MingwLLVM)
	{
		auto flavor = getCompilerFlavor(inPath);
		return fmt::format("LLVM Clang version {}{}", inVersion, flavor);
	}
	else
	{
		return fmt::format("LLVM Clang version {}", inVersion);
	}
}

/*****************************************************************************/
ToolchainType BuildEnvironmentLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	const bool clang = String::contains(StringList{ "__clang__", "__clang_major__", "__clang_version__" }, inMacros);

#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	auto gnuType = BuildEnvironmentGNU::getToolchainTypeFromMacros(inMacros);
	const bool mingw = gnuType == ToolchainType::MingwGNU;
	if (clang && mingw)
	{
		m_type = ToolchainType::MingwLLVM;

		return ToolchainType::MingwLLVM;
	}
	else
#endif
		if (clang)
		return ToolchainType::LLVM;

	return ToolchainType::Unknown;
}

/*****************************************************************************/
bool BuildEnvironmentLLVM::validateArchitectureFromInput()
{
	return IBuildEnvironment::validateArchitectureFromInput();
}

/*****************************************************************************/
bool BuildEnvironmentLLVM::readArchitectureTripleFromCompiler()
{
	const auto& compiler = m_state.toolchain.compilerCxxAny().path;

	if (compiler.empty())
		return false;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedArch;
	if (sourceCache.archRequriesUpdate(compiler, cachedArch))
	{
		auto targetArch = m_state.info.targetArchitectureTriple();

		bool emptyInputArch = m_state.inputs.targetArchitecture().empty();
		if (emptyInputArch || !String::contains('-', targetArch))
		{
			cachedArch = Process::runOutput({ compiler, "-dumpmachine" });
			auto firstDash = cachedArch.find_first_of('-');

			bool valid = !cachedArch.empty() && firstDash != std::string::npos;
			if (!valid)
				return false;

			auto suffix = cachedArch.substr(firstDash);
#if defined(CHALET_LINUX)
			Arch::Cpu arch = m_state.info.targetArchitecture();
			if (arch == Arch::Cpu::ARMHF)
			{
				suffix += "eabihf";
				targetArch = "arm";
			}
			else if (arch == Arch::Cpu::ARM)
			{
				suffix += "eabi";
				targetArch = "arm";
			}
			else if (arch == Arch::Cpu::ARM64)
			{
				targetArch = "aarch64";
			}
#endif
			cachedArch = fmt::format("{}{}", targetArch, suffix);

#if defined(CHALET_LINUX)
			auto searchPathA = fmt::format("/usr/lib/gcc/{}", cachedArch);
			auto searchPathB = fmt::format("/usr/lib/gcc-cross/{}", cachedArch);

			bool found = Files::pathExists(searchPathA) || Files::pathExists(searchPathB);
			if (!found && String::startsWith("-pc-linux-gnu", suffix))
			{
				suffix = suffix.substr(3);
				cachedArch = fmt::format("{}{}", targetArch, suffix);

				searchPathA = fmt::format("/usr/lib/gcc/{}", cachedArch);
				searchPathB = fmt::format("/usr/lib/gcc-cross/{}", cachedArch);

				found = Files::pathExists(searchPathA) || Files::pathExists(searchPathB);
			}

			if (!found)
			{
				cachedArch.clear();
			}
#endif
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto darwin = cachedArch.find("apple-darwin");
			if (darwin != std::string::npos)
			{
				cachedArch = cachedArch.substr(0, darwin + 12);
			}
#endif
		}
		else
		{
			cachedArch = targetArch;
		}
	}

	if (cachedArch.empty())
		return false;

	m_state.info.setTargetArchitecture(cachedArch);
	sourceCache.addArch(compiler, cachedArch);

	m_isWindowsTarget = String::contains(StringList{ "windows", "win32", "msvc", "mingw32", "w64" }, m_state.info.targetArchitectureTriple());
	m_isEmbeddedTarget = String::contains(StringList{ "-none-eabi" }, m_state.info.targetArchitectureTriple());

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentLLVM::populateSupportedFlags(const std::string& inExecutable)
{
	StringList cmd{ inExecutable, "-cc1", "--help" };
	parseSupportedFlagsFromHelpList(cmd);
	return true;
}

/*****************************************************************************/
void BuildEnvironmentLLVM::parseSupportedFlagsFromHelpList(const StringList& inCommand)
{
	std::string raw = Process::runOutput(inCommand);
	auto split = String::split(raw, '\n');

	for (auto& line : split)
	{
		auto beg = line.find_first_not_of(' ');
		auto end = line.find_first_of('=', beg);
		if (end == std::string::npos)
		{
			end = line.find_first_of('<', beg);
			if (end == std::string::npos)
			{
				end = line.find_first_of(' ', beg);
			}
		}

		if (beg != std::string::npos && end != std::string::npos)
		{
			line = line.substr(beg, end - beg);
		}

		if (line.back() == ' ')
			line.pop_back();
		if (line.back() == ',')
			line.pop_back();

		if (String::startsWith('-', line))
		{
			if (String::contains('\t', line))
			{
				auto afterTab = line.find_last_of('\t');
				if (afterTab != std::string::npos)
				{
					std::string secondFlag = line.substr(afterTab);

					if (String::startsWith('-', secondFlag))
					{
						if (secondFlag.back() == ' ')
							secondFlag.pop_back();
						if (secondFlag.back() == ',')
							secondFlag.pop_back();

						if (m_supportedFlags.find(secondFlag) == m_supportedFlags.end())
							m_supportedFlags.emplace(String::toLowerCase(secondFlag), true);
					}
				}

				end = line.find_first_of('"');
				if (end == std::string::npos)
				{
					end = line.find_first_of(' ');
				}

				line = line.substr(beg, end - beg);

				if (String::startsWith('-', line))
				{
					if (line.back() == ' ')
						line.pop_back();
					if (line.back() == ',')
						line.pop_back();

					if (m_supportedFlags.find(line) == m_supportedFlags.end())
						m_supportedFlags.emplace(String::toLowerCase(line), true);
				}
			}
			else
			{
				if (m_supportedFlags.find(line) == m_supportedFlags.end())
					m_supportedFlags.emplace(String::toLowerCase(line), true);
			}
		}
	}

	// std::string supported;
	// for (auto& [flag, _] : m_supportedFlags)
	// {
	// 	supported += flag + '\n';
	// }
	// std::ofstream("clang_flags.txt") << supported;
}
}