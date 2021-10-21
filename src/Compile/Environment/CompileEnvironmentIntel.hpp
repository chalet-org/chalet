/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_INTEL_HPP
#define CHALET_COMPILE_ENVIRONMENT_INTEL_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"

namespace chalet
{
struct CompileEnvironmentIntel final : ICompileEnvironment
{
	explicit CompileEnvironmentIntel(const CommandLineInputs& inInputs, BuildState& inState, const ToolchainType inType);

	virtual ToolchainType type() const noexcept final;

protected:
	virtual bool createFromVersion(const std::string& inVersion) final;

private:
	void makeArchitectureCorrections();
	bool saveIntelEnvironment() const;

	const std::string kVarsId;

	std::string m_varsFileOriginal;
	std::string m_varsFileIntel;
	std::string m_varsFileIntelDelta;
	std::string m_intelSetVars;

	ToolchainType m_type;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_INTEL_HPP
