/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_TOOLS_HPP
#define CHALET_COMPILER_TOOLS_HPP

#include "Compile/BuildPathStyle.hpp"
#include "Compile/CodeLanguage.hpp"
#include "Compile/CompilerInfo.hpp"
#include "Compile/Strategy/StrategyType.hpp"

namespace chalet
{
struct ICompileEnvironment;

struct CompilerTools
{
	bool initialize(ICompileEnvironment& inEnvironment);
	bool validate();

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

	const std::string& compilerCxx() const noexcept;

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

private:
	std::string m_archiver;
	std::string m_cmake;
	std::string m_compilerC;
	std::string m_compilerCpp;
	std::string m_compilerWindowsResource;
	std::string m_disassembler;
	std::string m_linker;
	std::string m_make;
	std::string m_ninja;
	std::string m_profiler;

	CompilerInfo m_compilerCppInfo;
	CompilerInfo m_compilerCInfo;

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

	StrategyType m_strategy = StrategyType::None;
	BuildPathStyle m_buildPathStyle = BuildPathStyle::None;

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
};
}

#endif // CHALET_COMPILER_TOOLS_HPP
