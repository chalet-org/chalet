/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Core/Arch.hpp"

#include "Cache/SourceCache.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
namespace
{
static struct
{
	const Dictionary<BuildPathStyle> buildPathStyles{
		{ "target-triple", BuildPathStyle::TargetTriple },
		{ "toolchain-name", BuildPathStyle::ToolchainName },
		{ "configuration", BuildPathStyle::Configuration },
		{ "arch-configuration", BuildPathStyle::ArchConfiguration },
	};

	const Dictionary<StrategyType> strategyTypes{
		{ "makefile", StrategyType::Makefile },
		{ "ninja", StrategyType::Ninja },
		{ "native-experimental", StrategyType::Native },
	};
} state;
}

/*****************************************************************************/
bool CompilerTools::initialize(ICompileEnvironment& inEnvironment)
{
	auto getCompilerInfo = [&](CompilerInfo& outInfo) -> bool {
		if (!outInfo.description.empty())
			return false;

		if (!outInfo.path.empty() && Commands::pathExists(outInfo.path))
		{
			if (!inEnvironment.getCompilerInfoFromExecutable(outInfo))
				return false;
		}

		return true;
	};

	if (!getCompilerInfo(m_compilerCpp))
		return false;

	auto baseFolderC = String::getPathFolder(m_compilerC.path);
	auto baseFolderCpp = String::getPathFolder(m_compilerCpp.path);
	if (String::equals(baseFolderC, baseFolderCpp))
	{
		m_compilerC = m_compilerCpp;
	}
	else
	{
		if (!getCompilerInfo(m_compilerC))
			return false;
	}

	const auto& version = m_compilerCpp.version.empty() ? m_compilerC.version : m_compilerCpp.version;
	if (m_version.empty() || (inEnvironment.compilerVersionIsToolchainVersion() && m_version != version))
	{
		m_version = version;
	}

	return true;
}

/*****************************************************************************/
bool CompilerTools::validate()
{
	bool valid = true;
	if (m_strategy == StrategyType::None)
	{
		Diagnostic::error("Invalid toolchain strategy type: {}", m_strategyString);
		valid = false;
	}

	if (m_buildPathStyle == BuildPathStyle::None)
	{
		Diagnostic::error("Invalid toolchain buildPathStyle type: {}", m_buildPathStyleString);
		valid = false;
	}

	return valid;
}

/*****************************************************************************/
void CompilerTools::fetchMakeVersion(SourceCache& inCache)
{
	if (!m_make.empty() && m_makeVersionMajor == 0 && m_makeVersionMinor == 0)
	{
		std::string version;
		if (inCache.versionRequriesUpdate(m_make, version))
		{
			version = Commands::subprocessOutput({ m_make, "--version" });
			version = Commands::isolateVersion(version);
		}

		if (!version.empty())
		{
			inCache.addVersion(m_make, version);

			auto vals = String::split(version, '.');
			if (vals.size() == 2)
			{
				m_makeVersionMajor = std::stoi(vals[0]);
				m_makeVersionMinor = std::stoi(vals[1]);
			}

			m_makeIsJom = String::endsWith("jom.exe", m_make);
			m_makeIsNMake = String::endsWith("nmake.exe", m_make) || m_makeIsJom;
		}
	}
}

/*****************************************************************************/
bool CompilerTools::fetchCmakeVersion(SourceCache& inCache)
{
	if (!m_cmake.empty() && m_cmakeVersionMajor == 0 && m_cmakeVersionMinor == 0)
	{
		m_cmakeAvailable = false;
		std::string version;
		if (inCache.versionRequriesUpdate(m_cmake, version))
		{
			version = Commands::subprocessOutput({ m_cmake, "--version" });
			version = Commands::isolateVersion(version);
		}

		if (!version.empty())
		{
			inCache.addVersion(m_cmake, version);

			auto vals = String::split(version, '.');
			if (vals.size() == 3)
			{
				m_cmakeAvailable = true;
				m_cmakeVersionMajor = std::stoi(vals[0]);
				m_cmakeVersionMinor = std::stoi(vals[1]);
				m_cmakeVersionPatch = std::stoi(vals[2]);
			}
		}
	}

	return m_cmakeAvailable;
}

/*****************************************************************************/
void CompilerTools::fetchNinjaVersion(SourceCache& inCache)
{
	if (!m_ninja.empty() && m_ninjaVersionMajor == 0 && m_ninjaVersionMinor == 0)
	{
		m_ninjaAvailable = false;
		std::string version;
		if (inCache.versionRequriesUpdate(m_ninja, version))
		{
			version = Commands::subprocessOutput({ m_ninja, "--version" });
			version = Commands::isolateVersion(version);
		}

		if (!version.empty())
		{
			inCache.addVersion(m_ninja, version);

			auto vals = String::split(version, '.');
			if (vals.size() >= 3)
			{
				m_ninjaAvailable = true;
				m_ninjaVersionMajor = std::stoi(vals[0]);
				m_ninjaVersionMinor = std::stoi(vals[1]);
				m_ninjaVersionPatch = std::stoi(vals[2]);

				if (vals.size() > 3)
				{
					// .git (master build)
					m_ninjaVersionPatch++;
				}
			}
		}
	}
}

/*****************************************************************************/
StrategyType CompilerTools::strategy() const noexcept
{
	return m_strategy;
}

const std::string& CompilerTools::strategyString() const noexcept
{
	return m_strategyString;
}

void CompilerTools::setStrategy(const std::string& inValue) noexcept
{
	m_strategyString = inValue;

	if (state.strategyTypes.find(inValue) != state.strategyTypes.end())
		m_strategy = state.strategyTypes.at(inValue);
	else
		m_strategy = StrategyType::None;
}

/*****************************************************************************/
BuildPathStyle CompilerTools::buildPathStyle() const noexcept
{
	return m_buildPathStyle;
}

const std::string& CompilerTools::buildPathStyleString() const noexcept
{
	return m_buildPathStyleString;
}

void CompilerTools::setBuildPathStyle(const std::string& inValue) noexcept
{
	m_buildPathStyleString = inValue;

	if (state.buildPathStyles.find(inValue) != state.buildPathStyles.end())
		m_buildPathStyle = state.buildPathStyles.at(inValue);
	else
		m_buildPathStyle = BuildPathStyle::None;
}

/*****************************************************************************/
const std::string& CompilerTools::version() const noexcept
{
	return m_version;
}
void CompilerTools::setVersion(const std::string& inValue) noexcept
{
	m_version = inValue;
}

/*****************************************************************************/
const CompilerInfo& CompilerTools::compilerCxx(const CodeLanguage inLang) const noexcept
{
	if (inLang == CodeLanguage::C)
		return m_compilerC;
	else
		return m_compilerCpp;
}

/*****************************************************************************/
const CompilerInfo& CompilerTools::compilerCxxAny() const noexcept
{
	if (!m_compilerC.path.empty())
		return m_compilerC;
	else
		return m_compilerCpp;
}

/*****************************************************************************/
const std::string& CompilerTools::archiver() const noexcept
{
	return m_archiver;
}
void CompilerTools::setArchiver(std::string&& inValue) noexcept
{
	m_archiver = std::move(inValue);

	m_isArchiverLibTool = String::endsWith("libtool", m_archiver);
}
bool CompilerTools::isArchiverLibTool() const noexcept
{
	return m_isArchiverLibTool;
}

/*****************************************************************************/
const CompilerInfo& CompilerTools::compilerCpp() const noexcept
{
	return m_compilerCpp;
}
void CompilerTools::setCompilerCpp(std::string&& inValue) noexcept
{
	m_compilerCpp.path = std::move(inValue);
}

/*****************************************************************************/
const CompilerInfo& CompilerTools::compilerC() const noexcept
{
	return m_compilerC;
}
void CompilerTools::setCompilerC(std::string&& inValue) noexcept
{
	m_compilerC.path = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::cmake() const noexcept
{
	return m_cmake;
}
void CompilerTools::setCmake(std::string&& inValue) noexcept
{
	m_cmake = std::move(inValue);
}
uint CompilerTools::cmakeVersionMajor() const noexcept
{
	return m_cmakeVersionMajor;
}
uint CompilerTools::cmakeVersionMinor() const noexcept
{
	return m_cmakeVersionMinor;
}
uint CompilerTools::cmakeVersionPatch() const noexcept
{
	return m_cmakeVersionPatch;
}
bool CompilerTools::cmakeAvailable() const noexcept
{
	return m_cmakeAvailable;
}

/*****************************************************************************/
const std::string& CompilerTools::linker() const noexcept
{
	return m_linker;
}
void CompilerTools::setLinker(std::string&& inValue) noexcept
{
	m_linker = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::make() const noexcept
{
	return m_make;
}
void CompilerTools::setMake(std::string&& inValue) noexcept
{
	m_make = std::move(inValue);
}

uint CompilerTools::makeVersionMajor() const noexcept
{
	return m_makeVersionMajor;
}
uint CompilerTools::makeVersionMinor() const noexcept
{
	return m_makeVersionMinor;
}

bool CompilerTools::makeIsNMake() const noexcept
{
	return m_makeIsNMake;
}

bool CompilerTools::makeIsJom() const noexcept
{
	return m_makeIsJom;
}

/*****************************************************************************/
const std::string& CompilerTools::ninja() const noexcept
{
	return m_ninja;
}
void CompilerTools::setNinja(std::string&& inValue) noexcept
{
	m_ninja = std::move(inValue);
}
uint CompilerTools::ninjaVersionMajor() const noexcept
{
	return m_ninjaVersionMajor;
}
uint CompilerTools::ninjaVersionMinor() const noexcept
{
	return m_ninjaVersionMinor;
}
uint CompilerTools::ninjaVersionPatch() const noexcept
{
	return m_ninjaVersionPatch;
}
bool CompilerTools::ninjaAvailable() const noexcept
{
	return m_ninjaAvailable;
}

/*****************************************************************************/
const std::string& CompilerTools::profiler() const noexcept
{
	return m_profiler;
}
void CompilerTools::setProfiler(std::string&& inValue) noexcept
{
	m_profiler = std::move(inValue);

#if defined(CHALET_WIN32)
	m_isProfilerGprof = String::endsWith("gprof.exe", m_profiler);
#else
	m_isProfilerGprof = String::endsWith("gprof", m_profiler);
#endif
}
bool CompilerTools::isProfilerGprof() const noexcept
{
	return m_isProfilerGprof;
}

/*****************************************************************************/
const std::string& CompilerTools::disassembler() const noexcept
{
	return m_disassembler;
}
void CompilerTools::setDisassembler(std::string&& inValue) noexcept
{
	m_disassembler = std::move(inValue);

#if defined(CHALET_MACOS)
	m_isDisassemblerOtool = String::endsWith("otool", m_disassembler);
#elif defined(CHALET_WIN32)
	m_isDisassemblerDumpBin = String::endsWith("dumpbin.exe", m_disassembler);
	m_isDisassemblerLLVMObjDump = String::endsWith("llvm-objdump.exe", m_disassembler);
#else
	m_isDisassemblerLLVMObjDump = String::endsWith("llvm-objdump", m_disassembler);
#endif
}
bool CompilerTools::isDisassemblerDumpBin() const noexcept
{
	return m_isDisassemblerDumpBin;
}
bool CompilerTools::isDisassemblerOtool() const noexcept
{
	return m_isDisassemblerOtool;
}
bool CompilerTools::isDisassemblerLLVMObjDump() const noexcept
{
	return m_isDisassemblerLLVMObjDump;
}

/*****************************************************************************/
const std::string& CompilerTools::compilerWindowsResource() const noexcept
{
	return m_compilerWindowsResource;
}
void CompilerTools::setCompilerWindowsResource(std::string&& inValue) noexcept
{
	m_compilerWindowsResource = std::move(inValue);

#if defined(CHALET_WIN32)
	m_isCompilerWindowsResourceLLVMRC = String::endsWith({ "llvm-rc.exe", "llvm-rc" }, m_compilerWindowsResource);
#else
	m_isCompilerWindowsResourceLLVMRC = String::endsWith("llvm-rc", m_compilerWindowsResource);
#endif
}
bool CompilerTools::isCompilerWindowsResourceLLVMRC() const noexcept
{
	return m_isCompilerWindowsResourceLLVMRC;
}

}
