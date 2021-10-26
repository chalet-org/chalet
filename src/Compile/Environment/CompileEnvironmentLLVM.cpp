/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentLLVM::CompileEnvironmentLLVM(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironmentGNU(inType, inInputs, inState)
{
}

/*****************************************************************************/
StringList CompileEnvironmentLLVM::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable, "-target", m_state.info.targetArchitectureTriple(), "-v" };
}

/*****************************************************************************/
std::string CompileEnvironmentLLVM::getFullCxxCompilerString(const std::string& inVersion) const
{
	return fmt::format("LLVM Clang C/C++ version {}", inVersion);
}

/*****************************************************************************/
ToolchainType CompileEnvironmentLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	const bool clang = String::contains({ "__clang__", "__clang_major__", "__clang_version__" }, inMacros);

#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	// const bool mingw = gnuType == ToolchainType::MingwGNU;
	// if (clang && mingw)
	// 	return ToolchainType::MingwLLVM;
	// else
#endif
	if (clang)
		return ToolchainType::LLVM;

	return ToolchainType::Unknown;
}

/*****************************************************************************/
bool CompileEnvironmentLLVM::makeArchitectureAdjustments()
{
	bool valid = false;

	const auto& compiler = m_state.toolchain.compilerCxx();
	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedArch;
	if (sourceCache.archRequriesUpdate(compiler, cachedArch))
	{
		Arch::Cpu targetArch = m_state.info.targetArchitecture();
		const auto& archTriple = m_state.info.targetArchitectureTriple();

		if (!m_inputs.targetArchitecture().empty())
		{
			auto results = Commands::subprocessOutput({ compiler, "-print-targets" });

			if (!String::contains("error:", results))
			{
				StringList arches64{ "x86-64", "x86_64", "x64" };
				StringList arches86{ "i686", "x86" };
				auto split = String::split(results, '\n');
				for (auto& line : split)
				{
					auto start = line.find_first_not_of(' ');
					auto end = line.find_first_of(' ', start);

					auto result = line.substr(start, end - start);
					if (targetArch == Arch::Cpu::X64)
					{
						if (String::equals(arches64, result))
							valid = true;
					}
					else if (targetArch == Arch::Cpu::X86)
					{
						if (String::equals(arches86, result))
							valid = true;
					}
					else
					{
						if (String::startsWith(result, archTriple))
							valid = true;
					}
				}
			}
		}

		if (!String::contains('-', archTriple))
		{
			cachedArch = Commands::subprocessOutput({ compiler, "-dumpmachine" });
			auto firstDash = cachedArch.find_first_of('-');
			if (!cachedArch.empty() && firstDash != std::string::npos)
			{
				cachedArch = fmt::format("{}{}", archTriple, cachedArch.substr(firstDash));
#if defined(CHALET_MACOS)
				// Strip out version in auto-detected mac triple
				auto darwin = cachedArch.find("apple-darwin");
				if (darwin != std::string::npos)
				{
					cachedArch = cachedArch.substr(0, darwin + 12);
				}
#endif
				valid = true;
			}
		}
		else
		{
			valid = true;
		}
	}
	else
	{
		valid = true;
	}

	m_state.info.setTargetArchitecture(cachedArch);
	sourceCache.addArch(compiler, cachedArch);

	return valid;
}

/*****************************************************************************/
bool CompileEnvironmentLLVM::validateArchitectureFromInput()
{
	return ICompileEnvironment::validateArchitectureFromInput();
}
}