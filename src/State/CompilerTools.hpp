/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_TOOLS_HPP
#define CHALET_COMPILER_TOOLS_HPP

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

struct CompilerTools
{
	explicit CompilerTools(const CommandLineInputs& inInputs, BuildState& inState);

	bool initialize(const BuildTargetList& inTargets, JsonFile& inConfigJson);
	void detectToolchainFromPaths();
	void fetchCompilerVersions();

	void fetchMakeVersion();
	bool fetchCmakeVersion();
	void fetchNinjaVersion();

	StrategyType strategy() const noexcept;
	void setStrategy(const std::string& inValue) noexcept;

	const std::string& compiler() const noexcept;

	const std::string& compilerVersionStringCpp() const noexcept;
	const std::string& compilerVersionStringC() const noexcept;
	const std::string& compilerDetectedArchCpp() const noexcept;
	const std::string& compilerDetectedArchC() const noexcept;

	const std::string& archiver() const noexcept;
	void setArchiver(std::string&& inValue) noexcept;
	bool isArchiverLibTool() const noexcept;

	const std::string& cpp() const noexcept;
	void setCpp(std::string&& inValue) noexcept;

	const std::string& cc() const noexcept;
	void setCc(std::string&& inValue) noexcept;

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

	const std::string& objdump() const noexcept;
	void setObjdump(std::string&& inValue) noexcept;

	const std::string& ninja() const noexcept;
	void setNinja(std::string&& inValue) noexcept;
	uint ninjaVersionMajor() const noexcept;
	uint ninjaVersionMinor() const noexcept;
	uint ninjaVersionPatch() const noexcept;
	bool ninjaAvailable() const noexcept;

	const std::string& rc() const noexcept;
	void setRc(std::string&& inValue) noexcept;

	std::string getRootPathVariable();

	CompilerConfig& getConfig(const CodeLanguage inLanguage) const;

private:
	bool initializeCompilerConfigs(const BuildTargetList& inTargets);
	bool updateToolchainCacheNode(JsonFile& inConfigJson);

	std::string parseVersionMSVC(const std::string& inExecutable, std::string& outArch) const;
	std::string parseVersionGNU(const std::string& inExecutable, std::string& outArch, const std::string_view inEol = "\n") const;

	const CommandLineInputs& m_inputs;
	BuildState& m_state;

	mutable std::unordered_map<CodeLanguage, std::unique_ptr<CompilerConfig>> m_configs;

	std::string m_archiver;
	std::string m_cpp;
	std::string m_cc;
	std::string m_cmake;
	std::string m_linker;
	std::string m_make;
	std::string m_ninja;
	std::string m_profiler;
	std::string m_objdump;
	std::string m_rc;

	std::string m_compilerVersionStringCpp;
	std::string m_compilerVersionStringC;
	std::string m_compilerDetectedArchCpp;
	std::string m_compilerDetectedArchC;

	uint m_cmakeVersionMajor = 0;
	uint m_cmakeVersionMinor = 0;
	uint m_cmakeVersionPatch = 0;

	uint m_makeVersionMajor = 0;
	uint m_makeVersionMinor = 0;

	uint m_ninjaVersionMajor = 0;
	uint m_ninjaVersionMinor = 0;
	uint m_ninjaVersionPatch = 0;

	StrategyType m_strategy = StrategyType::Makefile;

	bool m_isArchiverLibTool = false;
	bool m_isProfilerGprof = false;

	bool m_ccDetected = false;
	bool m_cmakeAvailable = false;
	bool m_ninjaAvailable = false;
	bool m_makeIsNMake = false;
	bool m_makeIsJom = false;
};
}

#endif // CHALET_COMPILER_TOOLS_HPP
