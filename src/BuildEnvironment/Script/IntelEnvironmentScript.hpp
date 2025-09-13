/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "BuildEnvironment/Script/IEnvironmentScript.hpp"
#include "BuildEnvironment/VisualStudioVersion.hpp"

namespace chalet
{
struct CommandLineInputs;

struct IntelEnvironmentScript final : public IEnvironmentScript
{
	explicit IntelEnvironmentScript(const CommandLineInputs& inInputs);

	virtual bool makeEnvironment(const BuildState& inState) final;

	bool isPreset() noexcept;

private:
	virtual bool saveEnvironmentFromScript() final;
	virtual StringList getAllowedArchitectures() final;

	virtual std::string getPathVariable(const std::string& inNewPath) const final;

	const CommandLineInputs& m_inputs;

	std::string m_intelSetVars;
	std::string m_intelSetVarsArch;

	u32 m_vsVersion = 0u;
};
}
