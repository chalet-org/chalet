/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_EMSCRIPTEN_ENVIRONMENT_SCRIPT_HPP
#define CHALET_EMSCRIPTEN_ENVIRONMENT_SCRIPT_HPP

#include "Compile/Environment/IEnvironmentScript.hpp"

namespace chalet
{
struct CommandLineInputs;

struct EmscriptenEnvironmentScript final : public IEnvironmentScript
{
	explicit EmscriptenEnvironmentScript();

	virtual bool makeEnvironment(const BuildState& inState) final;
	virtual void readEnvironmentVariablesFromDeltaFile() final;

	bool isPreset() noexcept;

private:
	virtual bool saveEnvironmentFromScript() final;
	virtual StringList getAllowedArchitectures() final;

	std::string m_emsdkEnv;
};
}

#endif // CHALET_EMSCRIPTEN_ENVIRONMENT_SCRIPT_HPP
