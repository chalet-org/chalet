/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Platform/Arch.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
namespace
{
Dictionary<BuildPathStyle> getBuildPathStyleTypes()
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
		{ "native", StrategyType::Native },
		{ "msbuild", StrategyType::MSBuild },
		{ "xcodebuild", StrategyType::XcodeBuild },
	};

	return ret;
}
}

/*****************************************************************************/
StringList CompilerTools::getToolchainStrategiesForSchema()
{
	StringList ret{
		"makefile",
		"ninja",
		"native",
		"msbuild",
		"xcodebuild",
	};

	return ret;
}

/*****************************************************************************/
StringList CompilerTools::getToolchainStrategies()
{
	StringList ret = getToolchainStrategiesForSchema();

#if !defined(CHALET_WIN32)
	List::removeIfExists(ret, std::string("msbuild"));
#endif
#if !defined(CHALET_MACOS)
	List::removeIfExists(ret, std::string("xcodebuild"));
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
bool CompilerTools::initialize(IBuildEnvironment& inEnvironment)
{
	auto getCompilerInfo = [&](CompilerInfo& outInfo) -> bool {
		if (!outInfo.description.empty())
			return false;

		if (!outInfo.path.empty() && Files::pathExists(outInfo.path))
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
		setVersion(version);
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
	// Note: These get validated in ToolchainSettingsJsonParser::makeToolchain
	//  this is a fallback...
	//
	if (m_strategy == StrategyType::None)
	{
		Diagnostic::error("Invalid toolchain strategy.");
		valid = false;
	}

	if (m_buildPathStyle == BuildPathStyle::None)
	{
		Diagnostic::error("Invalid toolchain buildPathStyle.");
		valid = false;
	}

	return valid;
}

/*****************************************************************************/
void CompilerTools::fetchMakeVersion(WorkspaceInternalCacheFile& inCache)
{
	if (!m_make.empty() && m_makeVersionMajor == 0 && m_makeVersionMinor == 0)
	{
		auto version = inCache.getDataValueFromPath(m_make, [this]() {
			std::string ret;
#if defined(CHALET_WIN32)
			if (makeIsJom())
			{
				ret = Process::runOutput({ m_make, "-version" });
			}
			else if (makeIsNMake())
			{
				ret = Process::runOutput({ m_make });

				auto split = String::split(ret, '\n');
				if (split.size() > 1)
					ret = split[1];
			}
			else
#endif
			{
				ret = Process::runOutput({ m_make, "--version" });
			}

			return Files::isolateVersion(ret);
		});

		if (!version.empty())
		{
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
bool CompilerTools::fetchCmakeVersion(WorkspaceInternalCacheFile& inCache)
{
	if (!m_cmake.empty() && m_cmakeVersionMajor == 0 && m_cmakeVersionMinor == 0)
	{
		m_cmakeAvailable = false;

		auto version = inCache.getDataValueFromPath(m_cmake, [this]() {
			auto ret = Process::runOutput({ m_cmake, "--version" });
			return Files::isolateVersion(ret);
		});

		if (!version.empty())
		{
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
bool CompilerTools::fetchMesonVersion(WorkspaceInternalCacheFile& inCache)
{
	if (!m_meson.empty() && m_mesonVersionMajor == 0 && m_mesonVersionMinor == 0)
	{
		m_mesonAvailable = false;

		auto version = inCache.getDataValueFromPath(m_meson, [this]() {
			auto ret = Process::runOutput({ m_meson, "--version" });
			return Files::isolateVersion(ret);
		});

		if (!version.empty())
		{
			auto vals = String::split(version, '.');
			if (vals.size() == 3)
			{
				m_mesonAvailable = true;
				m_mesonVersionMajor = std::stoi(vals[0]);
				m_mesonVersionMinor = std::stoi(vals[1]);
				m_mesonVersionPatch = std::stoi(vals[2]);
			}
		}
	}

	return m_mesonAvailable;
}

/*****************************************************************************/
void CompilerTools::fetchNinjaVersion(WorkspaceInternalCacheFile& inCache)
{
	if (!m_ninja.empty() && m_ninjaVersionMajor == 0 && m_ninjaVersionMinor == 0)
	{
		m_ninjaAvailable = false;

		auto version = inCache.getDataValueFromPath(m_ninja, [this]() {
			auto ret = Process::runOutput({ m_ninja, "--version" });
			return Files::isolateVersion(ret);
		});

		if (!version.empty())
		{
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

void CompilerTools::setStrategy(const std::string& inValue) noexcept
{
	auto strategyTypes = getStrategyTypes();
	if (strategyTypes.find(inValue) != strategyTypes.end())
		m_strategy = strategyTypes.at(inValue);
	else
		m_strategy = StrategyType::None;
}

std::string CompilerTools::getStrategyString() const
{
	auto strategyTypes = getStrategyTypes();
	for (auto& [name, type] : strategyTypes)
	{
		if (type == m_strategy)
			return name;
	}
	return std::string();
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

void CompilerTools::setBuildPathStyle(const std::string& inValue) noexcept
{
	auto styles = getBuildPathStyleTypes();
	if (styles.find(inValue) != styles.end())
		m_buildPathStyle = styles.at(inValue);
	else
		m_buildPathStyle = BuildPathStyle::None;
}

bool CompilerTools::buildPathStyleIsValid(const std::string& inValue) const
{
	auto styles = getBuildPathStyleTypes();
	return styles.find(inValue) != styles.end();
}

/*****************************************************************************/
CustomToolchainTreatAs CompilerTools::treatAs() const noexcept
{
	return m_treatAs;
}
void CompilerTools::setTreatAs(const CustomToolchainTreatAs inValue) noexcept
{
	m_treatAs = inValue;
}

/*****************************************************************************/
const std::string& CompilerTools::version() const noexcept
{
	return m_version;
}
void CompilerTools::setVersion(const std::string& inValue) noexcept
{
	m_version = inValue;

	m_toolchainVersionMajor = 0;
	m_toolchainVersionMajorMinor = 0;
	m_toolchainVersionPatch = 0;

	if (!m_version.empty())
	{
		auto split = String::split(m_version, '.');
		if (split.size() >= 1)
		{
			m_toolchainVersionMajorMinor = atoi(split[0].c_str());
			m_toolchainVersionMajor = m_toolchainVersionMajorMinor;
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
u32 CompilerTools::versionMajor() const noexcept
{
	return m_toolchainVersionMajor;
}

u32 CompilerTools::versionMajorMinor() const noexcept
{
	return m_toolchainVersionMajorMinor;
}

u32 CompilerTools::versionPatch() const noexcept
{
	return m_toolchainVersionPatch;
}

/*****************************************************************************/
const CompilerInfo& CompilerTools::compilerCxx(const CodeLanguage inLang) const noexcept
{
	if (inLang == CodeLanguage::C || inLang == CodeLanguage::ObjectiveC)
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
u32 CompilerTools::cmakeVersionMajor() const noexcept
{
	return m_cmakeVersionMajor;
}
u32 CompilerTools::cmakeVersionMinor() const noexcept
{
	return m_cmakeVersionMinor;
}
u32 CompilerTools::cmakeVersionPatch() const noexcept
{
	return m_cmakeVersionPatch;
}
bool CompilerTools::cmakeAvailable() const noexcept
{
	return m_cmakeAvailable;
}

/*****************************************************************************/
const std::string& CompilerTools::meson() const noexcept
{
	return m_meson;
}
void CompilerTools::setMeson(std::string&& inValue) noexcept
{
	m_meson = std::move(inValue);
}
u32 CompilerTools::mesonVersionMajor() const noexcept
{
	return m_mesonVersionMajor;
}
u32 CompilerTools::mesonVersionMinor() const noexcept
{
	return m_mesonVersionMinor;
}
u32 CompilerTools::mesonVersionPatch() const noexcept
{
	return m_mesonVersionPatch;
}
bool CompilerTools::mesonAvailable() const noexcept
{
	return m_mesonAvailable;
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
	m_makeIsMinGW = String::endsWith("mingw32-make.exe", lower);
}

u32 CompilerTools::makeVersionMajor() const noexcept
{
	return m_makeVersionMajor;
}
u32 CompilerTools::makeVersionMinor() const noexcept
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
bool CompilerTools::makeIsMinGW() const noexcept
{
	return m_makeIsMinGW;
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
u32 CompilerTools::ninjaVersionMajor() const noexcept
{
	return m_ninjaVersionMajor;
}
u32 CompilerTools::ninjaVersionMinor() const noexcept
{
	return m_ninjaVersionMinor;
}
u32 CompilerTools::ninjaVersionPatch() const noexcept
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
}

/*****************************************************************************/
bool CompilerTools::isDisassemblerDumpBin() const noexcept
{
#if defined(CHALET_WIN32)
	auto lower = String::toLowerCase(m_disassembler);
	return String::endsWith("dumpbin.exe", lower);
#else
	return false;
#endif
}

/*****************************************************************************/
bool CompilerTools::isDisassemblerOtool() const noexcept
{
#if defined(CHALET_MACOS)
	auto lower = String::toLowerCase(m_disassembler);
	return String::endsWith("otool", lower);
#else
	return false;
#endif
}
/*****************************************************************************/
bool CompilerTools::isDisassemblerLLVMObjDump() const noexcept
{
	auto lower = String::toLowerCase(m_disassembler);
#if defined(CHALET_WIN32)
	return String::endsWith("llvm-objdump.exe", lower);
#else
	return String::endsWith("llvm-objdump", lower);
#endif
}

/*****************************************************************************/
bool CompilerTools::isDisassemblerWasm2Wat() const noexcept
{
	auto lower = String::toLowerCase(m_disassembler);
#if defined(CHALET_WIN32)
	return String::endsWith("wasm2wat.exe", lower);
#else
	return String::endsWith("wasm2wat", lower);
#endif
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

bool CompilerTools::canCompileWindowsResources() const noexcept
{
	return !m_compilerWindowsResource.empty() && m_isWindowsTarget;
}

}
