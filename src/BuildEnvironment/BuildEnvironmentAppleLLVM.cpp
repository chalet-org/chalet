/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentAppleLLVM.hpp"

#include "State/BuildState.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentAppleLLVM::BuildEnvironmentAppleLLVM(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
std::string BuildEnvironmentAppleLLVM::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	UNUSED(inPath);
	return fmt::format("Apple Clang version {}", inVersion);
}

/*****************************************************************************/
ToolchainType BuildEnvironmentAppleLLVM::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	auto llvmType = BuildEnvironmentLLVM::getToolchainTypeFromMacros(inMacros);
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