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
	ICompileEnvironment(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentLLVM::type() const noexcept
{
	return ToolchainType::LLVM;
}

}