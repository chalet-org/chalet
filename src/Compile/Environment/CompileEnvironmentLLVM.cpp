/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentLLVM::CompileEnvironmentLLVM(const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironmentGNU(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentLLVM::type() const noexcept
{
	return ToolchainType::LLVM;
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
bool CompileEnvironmentLLVM::makeArchitectureAdjustments()
{
	Arch::Cpu targetArch = m_state.info.targetArchitecture();
	const auto& archTriple = m_state.info.targetArchitectureTriple();
	const auto& compiler = m_state.toolchain.compilerCxx();

	bool valid = false;
	if (!m_inputs.targetArchitecture().empty())
	{
		auto results = Commands::subprocessOutput({ compiler, "-print-targets" });
		if (!String::contains("error:", results))
		{
			auto split = String::split(results, "\n");
			for (auto& line : split)
			{
				auto start = line.find_first_not_of(' ');
				auto end = line.find_first_of(' ', start);

				auto result = line.substr(start, end - start);
				if (targetArch == Arch::Cpu::X64)
				{
					if (String::equals({ "x86-64", "x86_64", "x64" }, result))
						valid = true;
				}
				else if (targetArch == Arch::Cpu::X86)
				{
					if (String::equals({ "i686", "x86" }, result))
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
		auto result = Commands::subprocessOutput({ compiler, "-dumpmachine" });
		auto firstDash = result.find_first_of('-');
		if (!result.empty() && firstDash != std::string::npos)
		{
			result = fmt::format("{}{}", archTriple, result.substr(firstDash));
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto darwin = result.find("apple-darwin");
			if (darwin != std::string::npos)
			{
				result = result.substr(0, darwin + 12);
			}
#endif
			m_state.info.setTargetArchitecture(result);
			valid = true;
		}
	}
	else
	{
		valid = true;
	}

	return valid;
}

/*****************************************************************************/
bool CompileEnvironmentLLVM::validateArchitectureFromInput()
{
	return ICompileEnvironment::validateArchitectureFromInput();
}
}