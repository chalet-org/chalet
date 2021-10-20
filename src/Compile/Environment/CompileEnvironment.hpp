/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_ENVIRONMENT_HPP
#define CHALET_COMPILER_ENVIRONMENT_HPP

#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

class CompileEnvironment
{
public:
	explicit CompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);
	virtual ~CompileEnvironment() = default;

	[[nodiscard]] static Unique<CompileEnvironment> make(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

	const std::string& detectedVersion() const;

	virtual bool create(const std::string& inVersion = std::string());

protected:
	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	std::string m_detectedVersion;
};
}

#endif // CHALET_COMPILER_ENVIRONMENT_HPP
