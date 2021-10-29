/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_WIN_RESOURCE_LLVM_RC_HPP
#define CHALET_COMPILER_WIN_RESOURCE_LLVM_RC_HPP

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"

namespace chalet
{
struct CompilerWinResourceLLVMRC final : public CompilerWinResourceGNUWindRes
{
	explicit CompilerWinResourceLLVMRC(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency) override;
};
}

#endif // CHALET_COMPILER_WIN_RESOURCE_LLVM_RC_HPP
