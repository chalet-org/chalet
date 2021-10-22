/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
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
	if (String::equals("gcc", m_inputs.toolchainPreferenceName()))
	{
		const auto& arch = m_state.info.targetArchitectureString();
		m_inputs.setToolchainPreferenceName(fmt::format("{}-gcc", arch));
	}

	return true;
}
}