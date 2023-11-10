/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/BuildPathStyle.hpp"
#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerInfo.hpp"
#include "Compile/Strategy/StrategyType.hpp"
#include "State/CustomToolchainTreatAs.hpp"

namespace chalet
{
struct ICompileEnvironment;
struct SourceCache;

struct CompilerTools
{
	static StringList getToolchainStrategiesForSchema();
	static StringList getToolchainStrategies();
	static StringList getToolchainBuildPathStyles();

	bool initialize(ICompileEnvironment& inEnvironment);
	bool validate();

	void fetchMakeVersion(SourceCache& inCache);
	bool fetchCmakeVersion(SourceCache& inCache);
	void fetchNinjaVersion(SourceCache& inCache);

	StrategyType strategy() const noexcept;
	void setStrategy(const StrategyType inValue) noexcept;
	void setStrategy(const std::string& inValue) noexcept;
	bool strategyIsValid(const std::string& inValue) const;

	BuildPathStyle buildPathStyle() const noexcept;
	void setBuildPathStyle(const std::string& inValue) noexcept;
	bool buildPathStyleIsValid(const std::string& inValue) const;

	CustomToolchainTreatAs treatAs() const noexcept;
	void setTreatAs(const CustomToolchainTreatAs inValue) noexcept;

	const std::string& version() const noexcept;
	void setVersion(const std::string& inValue) noexcept;
	u32 versionMajorMinor() const noexcept;
	u32 versionPatch() const noexcept;

	const std::string& archiver() const noexcept;
	void setArchiver(std::string&& inValue) noexcept;

	const CompilerInfo& compilerCpp() const noexcept;
	void setCompilerCpp(std::string&& inValue) noexcept;

	const CompilerInfo& compilerC() const noexcept;
	void setCompilerC(std::string&& inValue) noexcept;

	const CompilerInfo& compilerCxx(const CodeLanguage inLang) const noexcept;
	const CompilerInfo& compilerCxxAny() const noexcept;

	const std::string& compilerWindowsResource() const noexcept;
	void setCompilerWindowsResource(std::string&& inValue) noexcept;
	bool isCompilerWindowsResourceLLVMRC() const noexcept;
	bool canCompileWindowsResources() const noexcept;

	const std::string& cmake() const noexcept;
	void setCmake(std::string&& inValue) noexcept;
	u32 cmakeVersionMajor() const noexcept;
	u32 cmakeVersionMinor() const noexcept;
	u32 cmakeVersionPatch() const noexcept;
	bool cmakeAvailable() const noexcept;

	const std::string& linker() const noexcept;
	void setLinker(std::string&& inValue) noexcept;

	const std::string& make() const noexcept;
	void setMake(std::string&& inValue) noexcept;
	u32 makeVersionMajor() const noexcept;
	u32 makeVersionMinor() const noexcept;
	bool makeIsNMake() const noexcept;
	bool makeIsJom() const noexcept;

	const std::string& profiler() const noexcept;
	void setProfiler(std::string&& inValue) noexcept;
	bool isProfilerGprof() const noexcept;
	bool isProfilerVSInstruments() const noexcept;

	const std::string& disassembler() const noexcept;
	void setDisassembler(std::string&& inValue) noexcept;
	bool isDisassemblerDumpBin() const noexcept;
	bool isDisassemblerOtool() const noexcept;
	bool isDisassemblerLLVMObjDump() const noexcept;
	bool isDisassemblerWasm2Wat() const noexcept;

	const std::string& ninja() const noexcept;
	void setNinja(std::string&& inValue) noexcept;
	u32 ninjaVersionMajor() const noexcept;
	u32 ninjaVersionMinor() const noexcept;
	u32 ninjaVersionPatch() const noexcept;
	bool ninjaAvailable() const noexcept;

private:
	bool parseVersionString(CompilerInfo& outInfo);

	std::string m_archiver;
	std::string m_cmake;
	std::string m_compilerWindowsResource;
	std::string m_disassembler;
	std::string m_linker;
	std::string m_make;
	std::string m_ninja;
	std::string m_profiler;

	CompilerInfo m_compilerCpp;
	CompilerInfo m_compilerC;

	std::string m_version;

	u32 m_toolchainVersionMajorMinor = 0;
	u32 m_toolchainVersionPatch = 0;

	u32 m_cmakeVersionMajor = 0;
	u32 m_cmakeVersionMinor = 0;
	u32 m_cmakeVersionPatch = 0;

	u32 m_makeVersionMajor = 0;
	u32 m_makeVersionMinor = 0;

	u32 m_ninjaVersionMajor = 0;
	u32 m_ninjaVersionMinor = 0;
	u32 m_ninjaVersionPatch = 0;

	StrategyType m_strategy = StrategyType::None;
	BuildPathStyle m_buildPathStyle = BuildPathStyle::None;
	CustomToolchainTreatAs m_treatAs = CustomToolchainTreatAs::None;

	bool m_isProfilerGprof = false;
	bool m_isProfilerVSInstruments = false;

	bool m_isWindowsTarget = false;
	bool m_isCompilerWindowsResourceLLVMRC = false;
	bool m_cmakeAvailable = false;
	bool m_ninjaAvailable = false;
	bool m_makeIsNMake = false;
	bool m_makeIsJom = false;
};
}
