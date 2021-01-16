/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileFactory.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Diagnostic.hpp"

#include "Compile/Strategy/CompileStrategyMakefile.hpp"
#include "Compile/Strategy/CompileStrategyNative.hpp"
#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Compile/Toolchain/CompileToolchainGNU.hpp"
#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] CompileStrategy CompileFactory::makeStrategy(const StrategyType inType, BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain)
{
	switch (inType)
	{
		case StrategyType::Makefile:
			return std::make_unique<CompileStrategyMakefile>(inState, inProject, inToolchain);
		case StrategyType::Ninja:
			return std::make_unique<CompileStrategyNinja>(inState, inProject, inToolchain);
		case StrategyType::Native:
			return std::make_unique<CompileStrategyNative>(inState, inProject, inToolchain);
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented StrategyType requested: ", static_cast<int>(inType)));
	return nullptr;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain CompileFactory::makeToolchain(const ToolchainType inType, const BuildState& inState, const ProjectConfiguration& inProject)
{
	switch (inType)
	{
		case ToolchainType::GNU:
			return std::make_unique<CompileToolchainGNU>(inState, inProject);
		case ToolchainType::LLVM:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject);
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented ToolchainType requested: ", static_cast<int>(inType)));
	return nullptr;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain CompileFactory::makeToolchain(const CppCompilerType inCompilerType, const BuildState& inState, const ProjectConfiguration& inProject)
{
	switch (inCompilerType)
	{
		case CppCompilerType::AppleClang:
		case CppCompilerType::Clang:
		case CppCompilerType::MingwClang:
		case CppCompilerType::EmScripten:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject);
		case CppCompilerType::Intel:
		case CppCompilerType::MingwGcc:
		case CppCompilerType::Gcc:
			return std::make_unique<CompileToolchainGNU>(inState, inProject);
		case CppCompilerType::VisualStudio:
		case CppCompilerType::Unknown:
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented ToolchainType requested: ", static_cast<int>(inCompilerType)));
	return nullptr;
}
}
