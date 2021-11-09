/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_INTEL_ENVIRONMENT_SCRIPT_HPP
#define CHALET_INTEL_ENVIRONMENT_SCRIPT_HPP

#include "Compile/Environment/IEnvironmentScript.hpp"
#include "Core/VisualStudioVersion.hpp"

namespace chalet
{
struct CommandLineInputs;

struct IntelEnvironmentScript final : public IEnvironmentScript
{
	explicit IntelEnvironmentScript(const CommandLineInputs& inInputs);

	virtual bool makeEnvironment(const BuildState& inState) final;
	virtual void readEnvironmentVariablesFromDeltaFile() final;

	bool isPreset() noexcept;

private:
	virtual bool saveEnvironmentFromScript() final;
	virtual StringList getAllowedArchitectures() final;

	const CommandLineInputs& m_inputs;

	std::string m_intelSetVars;
	std::string m_intelSetVarsArch;

	VisualStudioVersion m_vsVersion = VisualStudioVersion::None;
};
}

#endif // CHALET_INTEL_ENVIRONMENT_SCRIPT_HPP
