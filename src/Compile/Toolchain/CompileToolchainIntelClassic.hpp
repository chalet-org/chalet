/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_HPP
#define CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_HPP

#include "Compile/Toolchain/CompileToolchainGNU.hpp"

namespace chalet
{
struct CompileToolchainIntelClassic : CompileToolchainGNU
{
	explicit CompileToolchainIntelClassic(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig);

	virtual ToolchainType type() const noexcept override;

	virtual bool initialize() final;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) final;

protected:
	// Compiling
	virtual void addWarnings(StringList& outArgList) const final;
	virtual void addPchInclude(StringList& outArgList) const final;

	// Linking

	std::string m_pchSource;
	std::string m_pchMinusLocation;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_HPP
