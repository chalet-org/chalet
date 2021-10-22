/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentAppleLLVM.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentAppleLLVM::CompileEnvironmentAppleLLVM(const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironmentLLVM(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentAppleLLVM::type() const noexcept
{
	return ToolchainType::AppleLLVM;
}

/*****************************************************************************/
std::string CompileEnvironmentAppleLLVM::getFullCxxCompilerString(const std::string& inVersion) const
{
	return fmt::format("Apple Clang C/C++ version {}", inVersion);
}

}