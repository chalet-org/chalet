/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_GNU_HPP
#define CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_GNU_HPP

#include "Compile/Toolchain/CompileToolchainGNU.hpp"

namespace chalet
{
struct CompileToolchainIntelClassicGNU final : CompileToolchainGNU
{
	explicit CompileToolchainIntelClassicGNU(const BuildState& inState, const SourceTarget& inProject);

	virtual ToolchainType type() const noexcept final;

	virtual bool initialize() final;

	virtual StringList getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) final;

protected:
	virtual StringList getWarningExclusions() const final;
	// Compiling

	// Linking

	StringList m_warningExclusions;

	std::string m_pchSource;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_GNU_HPP
