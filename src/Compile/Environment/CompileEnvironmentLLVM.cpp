/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentLLVM.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"

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
	return ICompileEnvironment::makeArchitectureAdjustments();
}
}