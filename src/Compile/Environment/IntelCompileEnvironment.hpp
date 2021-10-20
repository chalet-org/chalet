/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_INTEL_COMPILE_ENVIRONMENT_HPP
#define CHALET_INTEL_COMPILE_ENVIRONMENT_HPP

#include "Compile/Environment/CompileEnvironment.hpp"

namespace chalet
{
class IntelCompileEnvironment final : public CompileEnvironment
{
public:
	explicit IntelCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

protected:
	virtual bool createFromVersion(const std::string& inVersion) final;

private:
	bool saveIntelEnvironment() const;

	const std::string kVarsId;

	std::string m_varsFileOriginal;
	std::string m_varsFileIntel;
	std::string m_varsFileIntelDelta;
	std::string m_intelSetVars;
};
}

#endif // CHALET_INTEL_COMPILE_ENVIRONMENT_HPP
