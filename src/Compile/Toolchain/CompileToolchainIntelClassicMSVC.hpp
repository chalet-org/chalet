/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_MSVC_HPP
#define CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_MSVC_HPP

#include "Compile/Toolchain/CompileToolchainVisualStudio.hpp"

namespace chalet
{
struct CompileToolchainIntelClassicMSVC final : CompileToolchainVisualStudio
{
	explicit CompileToolchainIntelClassicMSVC(const BuildState& inState, const SourceTarget& inProject);

	virtual ToolchainType type() const noexcept final;

	virtual bool initialize() final;

protected:
	virtual StringList getWarningExclusions() const final;

	// Compiling
	virtual void addIncludes(StringList& outArgList) const final;
	virtual void addDiagnosticsOption(StringList& outArgList) const final;

	// Linking

	StringList m_warningExclusions;

	std::string m_pchSource;
};
}

#endif // CHALET_COMPILE_TOOLCHAIN_INTEL_CLASSIC_MSVC_HPP
