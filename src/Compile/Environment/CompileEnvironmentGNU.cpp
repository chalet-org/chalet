/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentGNU::CompileEnvironmentGNU(const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentGNU::type() const noexcept
{
	return ToolchainType::GNU;
}

/*****************************************************************************/
StringList CompileEnvironmentGNU::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable, "-v" };
}

/*****************************************************************************/
std::string CompileEnvironmentGNU::getFullCxxCompilerString(const std::string& inVersion) const
{
	return fmt::format("GNU Compiler Collection C/C++ version {}", inVersion);
}

/*****************************************************************************/
bool CompileEnvironmentGNU::makeArchitectureAdjustments()
{
	const auto& archTriple = m_state.info.targetArchitectureTriple();
	const auto& compiler = m_state.toolchain.compilerCxx();

	if (m_inputs.targetArchitecture().empty() || !String::contains('-', archTriple))
	{
		auto result = Commands::subprocessOutput({ compiler, "-dumpmachine" });
#if defined(CHALET_MACOS)
		// Strip out version in auto-detected mac triple
		auto darwin = result.find("apple-darwin");
		if (darwin != std::string::npos)
		{
			result = result.substr(0, darwin + 12);
		}
#endif
		m_state.info.setTargetArchitecture(result);
	}
	else
	{
		// Pass along and hope for the best

		// if (!String::startsWith(archFromInput, archTriple))
		// 	return false;
	}

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentGNU::validateArchitectureFromInput()
{
	if (String::equals("gcc", m_inputs.toolchainPreferenceName()))
	{
		const auto& arch = m_state.info.targetArchitectureString();
		m_inputs.setToolchainPreferenceName(fmt::format("{}-gcc", arch));
	}

	return true;
}
}