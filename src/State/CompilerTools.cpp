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
Dictionary<BuildPathStyle> getBuildPathstyles()
{
	return {
		{ "target-triple", BuildPathStyle::TargetTriple },
		{ "toolchain-name", BuildPathStyle::ToolchainName },
		{ "configuration", BuildPathStyle::Configuration },
		{ "architecture", BuildPathStyle::ArchConfiguration },
	};
}

Dictionary<StrategyType> getStrategyTypes()
{
	Dictionary<StrategyType> ret{
		{ "makefile", StrategyType::Makefile },
		{ "ninja", StrategyType::Ninja },
		{ "native-experimental", StrategyType::Native },
	};

#if defined(CHALET_WIN32)
	ret.emplace("msbuild", StrategyType::MSBuild);
#endif

	return ret;
}
}

/*****************************************************************************/
StringList CompilerTools::getToolchainStrategies()
{
	StringList ret{
		"makefile",
		"ninja",
		"native-experimental",
	};

#if defined(CHALET_WIN32)
	ret.emplace_back("msbuild");
#endif

	return ret;
}

/*****************************************************************************/
StringList CompilerTools::getToolchainBuildPathStyles()
{
	return {
		"target-triple",
		"toolchain-name",
		"configuration",
		"architecture",
	};
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

			if (!parseVersionString(outInfo))
			{
				Diagnostic::error("There was an error parsing the compiler executable's version string.");
				return false;
			}
		}

		return true;
	};

	if (!getCompilerInfo(m_compilerCpp))
		return false;

	auto baseFolderC = String::getPathFolder(m_compilerC.path);
	auto baseFolderCpp = String::getPathFolder(m_compilerCpp.path);
	if (String::equals(baseFolderC, baseFolderCpp))
	{
		std::string tmp = std::move(m_compilerC.path);
		m_compilerC = m_compilerCpp;
		m_compilerC.path = std::move(tmp);
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

	m_isWindowsTarget = inEnvironment.isWindowsTarget();

	return true;
}

/*****************************************************************************/
bool CompilerTools::parseVersionString(CompilerInfo& outInfo)
{
	outInfo.versionMajorMinor = 0;
	outInfo.versionPatch = 0;

	const auto& version = outInfo.version;
	if (!version.empty())
	{
		auto split = String::split(version, '.');
		if (split.size() >= 1)
		{
			outInfo.versionMajorMinor = atoi(split[0].c_str());
			outInfo.versionMajorMinor *= 100;

			if (split.size() >= 2)
			{
				outInfo.versionMajorMinor += atoi(split[1].c_str());

				if (split.size() >= 3)
				{
					outInfo.versionPatch = atoi(split[2].c_str());
				}
			}
		}
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

void CompilerTools::setStrategy(const StrategyType inValue) noexcept
{
	m_strategy = inValue;
}

const std::string& CompilerTools::strategyString() const noexcept
{
	return m_strategyString;
}

void CompilerTools::setStrategy(const std::string& inValue) noexcept
{
	m_strategyString = inValue;

	auto strategyTypes = getStrategyTypes();
	if (strategyTypes.find(inValue) != strategyTypes.end())
		m_strategy = strategyTypes.at(inValue);
	else
		m_strategy = StrategyType::None;
}

bool CompilerTools::strategyIsValid(const std::string& inValue) const
{
	auto strategyTypes = getStrategyTypes();
	return strategyTypes.find(inValue) != strategyTypes.end();
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

	auto buildPathStyles = getBuildPathstyles();
	if (buildPathStyles.find(inValue) != buildPathStyles.end())
		m_buildPathStyle = buildPathStyles.at(inValue);
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

	m_toolchainVersionMajorMinor = 0;
	m_toolchainVersionPatch = 0;

	if (!m_version.empty())
	{
		auto split = String::split(m_version, '.');
		if (split.size() >= 1)
		{
			m_toolchainVersionMajorMinor = atoi(split[0].c_str());
			m_toolchainVersionMajorMinor *= 100;

			if (split.size() >= 2)
			{
				m_toolchainVersionMajorMinor += atoi(split[1].c_str());

				if (split.size() >= 3)
				{
					m_toolchainVersionPatch = atoi(split[2].c_str());
				}
			}
		}
	}
}

/*****************************************************************************/
uint CompilerTools::versionMajorMinor() const noexcept
{
	return m_toolchainVersionMajorMinor;
}

uint CompilerTools::versionPatch() const noexcept
{
	return m_toolchainVersionPatch;
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

	auto lower = String::toLowerCase(m_make);
	m_makeIsJom = String::endsWith("jom.exe", lower);
	m_makeIsNMake = String::endsWith("nmake.exe", lower) || m_makeIsJom;
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

	auto lower = String::toLowerCase(m_profiler);
#if defined(CHALET_WIN32)
	m_isProfilerGprof = String::endsWith("gprof.exe", lower);
	m_isProfilerVSInstruments = String::endsWith("vsinstr.exe", lower);
#else
	m_isProfilerGprof = String::endsWith("gprof", lower);
	m_isProfilerVSInstruments = false;
#endif
}
bool CompilerTools::isProfilerGprof() const noexcept
{
	return m_isProfilerGprof;
}
bool CompilerTools::isProfilerVSInstruments() const noexcept
{
	return m_isProfilerVSInstruments;
}

/*****************************************************************************/
const std::string& CompilerTools::disassembler() const noexcept
{
	return m_disassembler;
}
void CompilerTools::setDisassembler(std::string&& inValue) noexcept
{
	m_disassembler = std::move(inValue);

	auto lower = String::toLowerCase(m_disassembler);
#if defined(CHALET_MACOS)
	m_isDisassemblerOtool = String::endsWith("otool", lower);
#elif defined(CHALET_WIN32)
	m_isDisassemblerLLVMObjDump = String::endsWith("llvm-objdump.exe", lower);
	m_isDisassemblerDumpBin = String::endsWith("dumpbin.exe", lower);
#else
	m_isDisassemblerLLVMObjDump = String::endsWith("llvm-objdump", lower);
	m_isDisassemblerDumpBin = false;
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

	auto lower = String::toLowerCase(m_compilerWindowsResource);
#if defined(CHALET_WIN32)
	m_isCompilerWindowsResourceLLVMRC = String::endsWith(StringList{ "llvm-rc.exe", "llvm-rc" }, lower);
#else
	m_isCompilerWindowsResourceLLVMRC = String::endsWith("llvm-rc", lower);
#endif
}
bool CompilerTools::isCompilerWindowsResourceLLVMRC() const noexcept
{
	return m_isCompilerWindowsResourceLLVMRC;
}

bool CompilerTools::canCompilerWindowsResources() const noexcept
{
	return !m_compilerWindowsResource.empty() && m_isWindowsTarget;
}

}
