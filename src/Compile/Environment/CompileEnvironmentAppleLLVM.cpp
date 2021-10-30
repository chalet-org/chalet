/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentAppleLLVM.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentAppleLLVM::CompileEnvironmentAppleLLVM(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironmentLLVM(inType, inInputs, inState)
{
}

/*****************************************************************************/
std::string CompileEnvironmentAppleLLVM::getIdentifier() const noexcept
{
	return std::string("apple-llvm");
}

/*****************************************************************************/
std::string CompileEnvironmentAppleLLVM::getFullCxxCompilerString(const std::string& inVersion) const
{
	return fmt::format("Apple Clang version {}", inVersion);
}

/*****************************************************************************/
ToolchainType CompileEnvironmentAppleLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = CompileEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
	if (llvmType != ToolchainType::LLVM)
		return llvmType;

#if defined(CHALET_MACOS)
	const bool appleClang = String::contains("Apple LLVM", inMacros);

	if (appleClang)
		return ToolchainType::AppleLLVM;
#endif

	return ToolchainType::Unknown;
}

}