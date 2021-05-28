/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_FACTORY_HPP
#define CHALET_COMPILE_FACTORY_HPP

#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/ICompileStrategy.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

#include "BuildJson/Target/ProjectTarget.hpp"
#include "State/BuildState.hpp"

// Produces a compile strategy (Makefile / Ninja / Custom),
//  an object that describes the toolchain's compile commands (GNU/GCC & LLVM/Clang)

namespace chalet
{
namespace CompileFactory
{
[[nodiscard]] CompileStrategy makeStrategy(const StrategyType inType, BuildState& inState);

[[nodiscard]] CompileToolchain makeToolchain(const ToolchainType inType, const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig);

[[nodiscard]] CompileToolchain makeToolchain(const CppCompilerType inCompilerType, const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig);
}
}

#endif // CHALET_COMPILE_FACTORY_HPP
