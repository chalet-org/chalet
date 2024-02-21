/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentAppleLLVM.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentAppleLLVM::BuildEnvironmentAppleLLVM(const ToolchainType inType, BuildState& inState) :
	BuildEnvironmentLLVM(inType, inState)
{
}

/*****************************************************************************/
bool BuildEnvironmentAppleLLVM::supportsCppModules() const
{
	auto& inputFile = m_state.inputs.inputFile();
	auto& compiler = m_state.toolchain.compilerCpp();
	u32 versionMajorMinor = compiler.versionMajorMinor;
	if (versionMajorMinor < 1600)
	{
		Diagnostic::error("{}: C++ modules are only supported with Apple Clang versions >= 16.0.0 (found {})", inputFile, compiler.version);
		return false;
	}
	return true;
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