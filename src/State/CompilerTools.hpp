/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_TOOLS_HPP
#define CHALET_COMPILER_TOOLS_HPP

#include "Compile/BuildPathStyle.hpp"
#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerConfig.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "Compile/Toolchain/ToolchainType.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
class BuildState;
struct CommandLineInputs;
struct JsonFile;

struct CompilerInfo
{
	std::string path;
	std::string description;
	std::string version;
	std::string arch;
};

struct CompilerTools
{
	explicit CompilerTools(const CommandLineInputs& inInputs, BuildState& inState);

	bool initialize(const BuildTargetList& inTargets, JsonFile& inConfigJson);
	bool detectToolchainFromPaths();
	void fetchCompilerVersions();

	void fetchMakeVersion();
	bool fetchCmakeVersion();
	void fetchNinjaVersion();

	StrategyType strategy() const noexcept;
	const std::string& strategyString() const noexcept;
	void setStrategy(const std::string& inValue) noexcept;

	BuildPathStyle buildPathStyle() const noexcept;
	const std::string& buildPathStyleString() const noexcept;
	void setBuildPathStyle(const std::string& inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(const std::string& inValue) noexcept;

	const std::string& compilerCxx() const noexcept;

	const std::string& compilerDescriptionStringCpp() const noexcept;
	const std::string& compilerDescriptionStringC() const noexcept;
	const std::string& compilerDetectedArchCpp() const noexcept;
	const std::string& compilerDetectedArchC() const noexcept;

	const std::string& archiver() const noexcept;
	void setArchiver(std::string&& inValue) noexcept;
	bool isArchiverLibTool() const noexcept;

	const std::string& compilerCpp() const noexcept;
	void setCompilerCpp(std::string&& inValue) noexcept;

	const std::string& compilerC() const noexcept;
	void setCompilerC(std::string&& inValue) noexcept;

	const std::string& compilerWindowsResource() const noexcept;
	void setCompilerWindowsResource(std::string&& inValue) noexcept;
	bool isCompilerWindowsResourceLLVMRC() const noexcept;

	const std::string& cmake() const noexcept;
	void setCmake(std::string&& inValue) noexcept;
	uint cmakeVersionMajor() const noexcept;
	uint cmakeVersionMinor() const noexcept;
	uint cmakeVersionPatch() const noexcept;
	bool cmakeAvailable() const noexcept;

	const std::string& linker() const noexcept;
	void setLinker(std::string&& inValue) noexcept;

	const std::string& make() const noexcept;
	void setMake(std::string&& inValue) noexcept;
	uint makeVersionMajor() const noexcept;
	uint makeVersionMinor() const noexcept;
	bool makeIsNMake() const noexcept;
	bool makeIsJom() const noexcept;

	const std::string& profiler() const noexcept;
	void setProfiler(std::string&& inValue) noexcept;
	bool isProfilerGprof() const noexcept;

	const std::string& disassembler() const noexcept;
	void setDisassembler(std::string&& inValue) noexcept;
	bool isDisassemblerDumpBin() const noexcept;
	bool isDisassemblerOtool() const noexcept;
	bool isDisassemblerLLVMObjDump() const noexcept;

	const std::string& ninja() const noexcept;
	void setNinja(std::string&& inValue) noexcept;
	uint ninjaVersionMajor() const noexcept;
	uint ninjaVersionMinor() const noexcept;
	uint ninjaVersionPatch() const noexcept;
	bool ninjaAvailable() const noexcept;

	std::string getRootPathVariable();

	CompilerConfig& getConfig(const CodeLanguage inLanguage) const;

private:
	bool initializeCompilerConfigs(const BuildTargetList& inTargets);
	bool updateToolchainCacheNode(JsonFile& inConfigJson);
#if defined(CHALET_WIN32)
	bool detectTargetArchitectureMSVC();
#endif

	std::string parseVersionMSVC(CompilerInfo& outInfo) const;
	std::string parseVersionGNU(CompilerInfo& outInfo) const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	mutable std::unordered_map<CodeLanguage, std::unique_ptr<CompilerConfig>> m_configs;

	std::string m_archiver;
	std::string m_compilerWindowsResource;
	std::string m_cmake;
	std::string m_linker;
	std::string m_make;
	std::string m_ninja;
	std::string m_profiler;
	std::string m_disassembler;

	CompilerInfo m_compilerCpp;
	CompilerInfo m_compilerC;

	std::string m_strategyString;
	std::string m_buildPathStyleString;
	std::string m_version;

	uint m_cmakeVersionMajor = 0;
	uint m_cmakeVersionMinor = 0;
	uint m_cmakeVersionPatch = 0;

	uint m_makeVersionMajor = 0;
	uint m_makeVersionMinor = 0;

	uint m_ninjaVersionMajor = 0;
	uint m_ninjaVersionMinor = 0;
	uint m_ninjaVersionPatch = 0;

	StrategyType m_strategy = StrategyType::Makefile;
	BuildPathStyle m_buildPathStyle = BuildPathStyle::TargetTriple;

	bool m_isArchiverLibTool = false;
	bool m_isProfilerGprof = false;

	bool m_ccDetected = false;
	bool m_isDisassemblerDumpBin = false;
	bool m_isDisassemblerOtool = false;
	bool m_isDisassemblerLLVMObjDump = false;
	bool m_isCompilerWindowsResourceLLVMRC = false;
	bool m_cmakeAvailable = false;
	bool m_ninjaAvailable = false;
	bool m_makeIsNMake = false;
	bool m_makeIsJom = false;
#if defined(CHALET_WIN32)
	bool m_msvcArchitectureSet = false;
#endif
};
}

#endif // CHALET_COMPILER_TOOLS_HPP
