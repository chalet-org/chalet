/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_INTEL_LLVM_HPP
#define CHALET_COMPILE_TOOLCHAIN_INTEL_LLVM_HPP

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

namespace chalet
{
struct CompileToolchainIntelLLVM final : CompileToolchainLLVM
{
	explicit CompileToolchainIntelLLVM(const BuildState& inState, const SourceTarget& inProject);

	virtual ToolchainType type() const noexcept final;

	virtual bool initialize() final;

	virtual StringList getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) final;

protected:
	// Compiling

	// Linking
	virtual void addThreadModelLinkerOption(StringList& outArgList) const final;

	StringList m_warningExclusions;

	std::string m_pchSource;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_INTEL_LLVM_HPP
