/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_ENVIRONMENT_HPP
#define CHALET_COMPILER_ENVIRONMENT_HPP

#include "Compile/CompilerInfo.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

struct ICompileEnvironment;
using CompileEnvironment = Unique<ICompileEnvironment>;

struct ICompileEnvironment
{
	explicit ICompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);
	virtual ~ICompileEnvironment() = default;

	[[nodiscard]] static Unique<ICompileEnvironment> make(ToolchainType type, const CommandLineInputs& inInputs, BuildState& inState);
	static ToolchainType detectToolchainTypeFromPath(const std::string& inExecutable);

	virtual ToolchainType type() const noexcept = 0;
	virtual StringList getVersionCommand(const std::string& inExecutable) const = 0;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const = 0;
	virtual CompilerInfo getCompilerInfoFromExecutable(const std::string& inExecutable) const = 0;

	virtual bool makeArchitectureAdjustments();
	virtual bool compilerVersionIsToolchainVersion() const;

	const std::string& detectedVersion() const;

	bool create(const std::string& inVersion = std::string());

protected:
	virtual bool createFromVersion(const std::string& inVersion);
	virtual bool validateArchitectureFromInput();

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