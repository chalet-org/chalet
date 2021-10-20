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

	bool create(const std::string& inVersion = std::string());

protected:
	virtual bool createFromVersion(const std::string& inVersion);

	std::string getVarsPath(const std::string& inId) const;
	bool saveOriginalEnvironment(const std::string& inOutputFile) const;
	void createEnvironmentDelta(const std::string& inOriginalFile, const std::string& inCompilerFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine) const;
	void cacheEnvironmentDelta(const std::string& inDeltaFile);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	Dictionary<std::string> m_variables;

	std::string m_detectedVersion;
	std::string m_path;

private:
	bool m_initialized = false;
};
}

#endif // CHALET_COMPILER_ENVIRONMENT_HPP
