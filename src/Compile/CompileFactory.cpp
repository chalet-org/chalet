/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileFactory.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Diagnostic.hpp"

#include "Compile/Strategy/CompileStrategyMakefile.hpp"
#include "Compile/Strategy/CompileStrategyNative.hpp"
// #include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Compile/Toolchain/CompileToolchainApple.hpp"
#include "Compile/Toolchain/CompileToolchainGNU.hpp"
#include "Compile/Toolchain/CompileToolchainLLVM.hpp"
#include "Compile/Toolchain/CompileToolchainMSVC.hpp"

namespace chalet
{
/*****************************************************************************/
[[nodiscard]] CompileStrategy CompileFactory::makeStrategy(const StrategyType inType, BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain)
{
	switch (inType)
	{
		case StrategyType::Makefile:
			return std::make_unique<CompileStrategyMakefile>(inState, inProject, inToolchain);
		// case StrategyType::Ninja:
		// 	return std::make_unique<CompileStrategyNinja>(inState, inProject, inToolchain);
		case StrategyType::Native:
			return std::make_unique<CompileStrategyNative>(inState, inProject, inToolchain);
		case StrategyType::Ninja:
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented StrategyType requested: ", static_cast<int>(inType)));
	return nullptr;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain CompileFactory::makeToolchain(const ToolchainType inType, const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig)
{
	switch (inType)
	{
		case ToolchainType::Apple:
			return std::make_unique<CompileToolchainApple>(inState, inProject, inConfig);
		case ToolchainType::LLVM:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject, inConfig);
		case ToolchainType::GNU:
			return std::make_unique<CompileToolchainGNU>(inState, inProject, inConfig);
		case ToolchainType::MSVC:
			return std::make_unique<CompileToolchainMSVC>(inState, inProject, inConfig);
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented ToolchainType requested: ", static_cast<int>(inType)));
	return nullptr;
}

/*****************************************************************************/
[[nodiscard]] CompileToolchain CompileFactory::makeToolchain(const CppCompilerType inCompilerType, const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig)
{
	switch (inCompilerType)
	{
		case CppCompilerType::AppleClang:
			return std::make_unique<CompileToolchainApple>(inState, inProject, inConfig);
		case CppCompilerType::Clang:
		case CppCompilerType::MingwClang:
		case CppCompilerType::EmScripten:
			return std::make_unique<CompileToolchainLLVM>(inState, inProject, inConfig);
		case CppCompilerType::Intel:
		case CppCompilerType::MingwGcc:
		case CppCompilerType::Gcc:
			return std::make_unique<CompileToolchainGNU>(inState, inProject, inConfig);
		case CppCompilerType::VisualStudio:
			return std::make_unique<CompileToolchainMSVC>(inState, inProject, inConfig);
		case CppCompilerType::Unknown:
		default:
			break;
	}

	Diagnostic::errorAbort(fmt::format("Unimplemented ToolchainType requested: ", static_cast<int>(inCompilerType)));
	return nullptr;
}
}
