/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_ENVIRONMENT_HPP
#define CHALET_COMPILER_ENVIRONMENT_HPP

#include "Compile/CompilerInfo.hpp"
#include "Compile/CompilerPathStructure.hpp"
#include "Compile/ToolchainType.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;

struct ICompileEnvironment;
using CompileEnvironment = Unique<ICompileEnvironment>;

struct ICompileEnvironment
{
	virtual ~ICompileEnvironment() = default;

	static bool saveOriginalEnvironment(const std::string& inOutputFile);

	const std::string& identifier() const noexcept;
	ToolchainType type() const noexcept;

	bool isWindowsClang() const noexcept;
	bool isClang() const noexcept;
	bool isAppleClang() const noexcept;
	bool isGcc() const noexcept;
	bool isIntelClassic() const noexcept;
	bool isMingw() const noexcept;
	bool isMingwGcc() const noexcept;
	bool isMsvc() const noexcept;
	bool isClangOrMsvc() const noexcept;

	const std::string& detectedVersion() const;
	bool isCompilerFlagSupported(const std::string& inFlag) const;

	bool ouptuttedDescription() const noexcept;

protected:
	friend class BuildState;
	friend struct CompilerTools;

	explicit ICompileEnvironment(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState);

	[[nodiscard]] static Unique<ICompileEnvironment> make(ToolchainType type, const CommandLineInputs& inInputs, BuildState& inState);
	static ToolchainType detectToolchainTypeFromPath(const std::string& inExecutable);

	virtual std::string getIdentifier() const noexcept = 0;
	virtual StringList getVersionCommand(const std::string& inExecutable) const = 0;
	virtual std::string getFullCxxCompilerString(const std::string& inVersion) const = 0;
	virtual bool verifyToolchain() = 0;
	virtual bool getCompilerVersionAndDescription(CompilerInfo& outInfo) const = 0;
	virtual std::vector<CompilerPathStructure> getValidCompilerPaths() const = 0;

	virtual bool readArchitectureTripleFromCompiler();
	virtual bool compilerVersionIsToolchainVersion() const;
	virtual bool createFromVersion(const std::string& inVersion);
	virtual bool validateArchitectureFromInput();
	virtual bool populateSupportedFlags(const std::string& inExecutable);

	bool create(const std::string& inVersion = std::string());
	bool getCompilerPaths(CompilerInfo& outInfo) const;
	bool getCompilerInfoFromExecutable(CompilerInfo& outInfo);
	bool makeSupportedCompilerFlags(const std::string& inExecutable);

	std::string getVarsPath() const;
	static void createEnvironmentDelta(const std::string& inOriginalFile, const std::string& inCompilerFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine);
	static void cacheEnvironmentDelta(const std::string& inDeltaFile, Dictionary<std::string>& outVariables);

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	Dictionary<bool> m_supportedFlags;

	std::string m_detectedVersion;

	const ToolchainType m_type;

	bool m_ouptuttedDescription = false;

private:
	std::string m_identifier;

	bool m_initialized = false;
};
}

#endif // CHALET_COMPILER_ENVIRONMENT_HPP
