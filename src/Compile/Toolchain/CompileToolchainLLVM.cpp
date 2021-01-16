/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

namespace chalet
{
/*****************************************************************************/
CompileToolchainLLVM::CompileToolchainLLVM(const BuildState& inState, const ProjectConfiguration& inProject) :
	CompileToolchainGNU(inState, inProject)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainLLVM::type() const
{
	return ToolchainType::LLVM;
}

}
