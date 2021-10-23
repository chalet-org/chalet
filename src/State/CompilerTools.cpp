/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildState.hpp"
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
/*****************************************************************************/
CompilerTools::CompilerTools(BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool CompilerTools::initialize(const BuildTargetList& inTargets)
{
	for (auto& target : inTargets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<SourceTarget&>(*target);
			auto language = project.language();

			if (m_configs.find(language) == m_configs.end())
			{
				m_configs.emplace(language, std::make_unique<CompilerConfig>(language, m_state));
			}
		}
	}

	for (auto& [_, config] : m_configs)
	{
		if (!config->configureCompilerPaths())
		{
			Diagnostic::error("Error configuring compiler paths.");
			return false;
		}

		if (!config->testCompilerMacros())
		{
			Diagnostic::error("Unimplemented or unknown compiler toolchain.");
			return false;
		}

		if (!config->getSupportedCompilerFlags())
		{
			auto exec = String::getPathFilename(config->compilerExecutable());
			Diagnostic::error("Error collecting supported compiler flags for '{}'.", exec);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CompilerTools::fetchCompilerVersions()
{
	auto createDescription = [&](const std::string& inPath, CompilerInfo& outInfo) {
		if (!outInfo.description.empty())
			return;

		if (!inPath.empty() && Commands::pathExists(inPath))
		{
			outInfo = m_state.environment->getCompilerInfoFromExecutable(inPath);
		}
	};

	createDescription(m_compilerCpp, m_compilerCppInfo);

	auto baseFolderC = String::getPathFolder(m_compilerC);
	auto baseFolderCpp = String::getPathFolder(m_compilerCpp);
	if (String::equals(baseFolderC, baseFolderCpp))
	{
		m_compilerCInfo = m_compilerCppInfo;
	}

	createDescription(m_compilerC, m_compilerCInfo);

	const auto& version = m_compilerCppInfo.version.empty() ? m_compilerCInfo.version : m_compilerCppInfo.version;
	if (m_version.empty() || (m_state.environment->compilerVersionIsToolchainVersion() && m_version != version))
	{
		m_version = version;
	}

	return true;
}

/*****************************************************************************/
void CompilerTools::fetchMakeVersion()
{
	if (!m_make.empty() && m_makeVersionMajor == 0 && m_makeVersionMinor == 0)
	{
		if (Commands::pathExists(m_make))
		{
			std::string version = Commands::subprocessOutput({ m_make, "--version" });
			version = Commands::isolateVersion(version);

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
bool CompilerTools::fetchCmakeVersion()
{
	if (!m_cmake.empty() && m_cmakeVersionMajor == 0 && m_cmakeVersionMinor == 0)
	{
		if (Commands::pathExists(m_cmake))
		{
			std::string version = Commands::subprocessOutput({ m_cmake, "--version" });
			m_cmakeAvailable = String::startsWith("cmake version ", version);

			version = Commands::isolateVersion(version);

			auto vals = String::split(version, '.');
			if (vals.size() == 3)
			{
				m_cmakeVersionMajor = std::stoi(vals[0]);
				m_cmakeVersionMinor = std::stoi(vals[1]);
				m_cmakeVersionPatch = std::stoi(vals[2]);
			}
		}
	}

	return m_cmakeAvailable;
}

/*****************************************************************************/
void CompilerTools::fetchNinjaVersion()
{
	if (!m_ninja.empty() && m_ninjaVersionMajor == 0 && m_ninjaVersionMinor == 0)
	{
		if (Commands::pathExists(m_ninja))
		{
			std::string version = Commands::subprocessOutput({ m_ninja, "--version" });
			version = Commands::isolateVersion(version);

			auto vals = String::split(version, '.');
			if (vals.size() >= 3)
			{
				m_ninjaVersionMajor = std::stoi(vals[0]);
				m_ninjaVersionMinor = std::stoi(vals[1]);
				m_ninjaVersionPatch = std::stoi(vals[2]);

				if (vals.size() > 3)
				{
					// .git (master build)
					m_ninjaVersionPatch++;
				}
			}

			m_ninjaAvailable = m_ninjaVersionMajor > 0 && m_ninjaVersionMinor > 0;
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
	if (String::equals("makefile", inValue))
	{
		m_strategy = StrategyType::Makefile;
	}
	else if (String::equals("native-experimental", inValue))
	{
		m_strategy = StrategyType::Native;
	}
	else if (String::equals("ninja", inValue))
	{
		m_strategy = StrategyType::Ninja;
	}
	else
	{
		Diagnostic::error("Invalid toolchain strategy type: {}", inValue);
	}
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
	if (String::equals("target-triple", inValue))
	{
		m_buildPathStyle = BuildPathStyle::TargetTriple;
	}
	else if (String::equals(inValue, "toolchain-name"))
	{
		m_buildPathStyle = BuildPathStyle::ToolchainName;
	}
	else if (String::equals(inValue, "configuration"))
	{
		m_buildPathStyle = BuildPathStyle::Configuration;
	}
	else if (String::equals(inValue, "arch-configuration"))
	{
		m_buildPathStyle = BuildPathStyle::ArchConfiguration;
	}
	else
	{
		Diagnostic::error("Invalid toolchain buildPathStyle type: {}", inValue);
	}
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
const std::string& CompilerTools::compilerCxx() const noexcept
{
	if (m_ccDetected)
		return m_compilerC;
	else
		return m_compilerCpp;
}

/*****************************************************************************/
const std::string& CompilerTools::compilerDescriptionStringCpp() const noexcept
{
	return m_compilerCppInfo.description;
}

const std::string& CompilerTools::compilerDescriptionStringC() const noexcept
{
	return m_compilerCInfo.description;
}

const std::string& CompilerTools::compilerDetectedArchCpp() const noexcept
{
	return m_compilerCppInfo.arch;
}
const std::string& CompilerTools::compilerDetectedArchC() const noexcept
{
	return m_compilerCInfo.arch;
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
const std::string& CompilerTools::compilerCpp() const noexcept
{
	return m_compilerCpp;
}
void CompilerTools::setCompilerCpp(std::string&& inValue) noexcept
{
	m_compilerCpp = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::compilerC() const noexcept
{
	return m_compilerC;
}
void CompilerTools::setCompilerC(std::string&& inValue) noexcept
{
	m_compilerC = std::move(inValue);
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

/*****************************************************************************/
/*std::string CompilerTools::getRootPathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	char separator = Path::getSeparator();
	auto pathList = String::split(originalPath, separator);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(m_compilerC); !List::contains(pathList, ccRoot))
		outList.emplace_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(m_compilerCpp); !List::contains(pathList, cppRoot))
		outList.emplace_back(std::move(cppRoot));

	for (auto& p : Path::getOSPaths())
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // probably not needed, but just in case

		if (!List::contains(pathList, path))
			outList.emplace_back(std::move(path));
	}

	for (auto& path : pathList)
	{
		List::addIfDoesNotExist(outList, std::move(path));
	}

	std::string ret = String::join(std::move(outList), separator);
	Path::sanitize(ret);

	return ret;
}*/

/*****************************************************************************/
CompilerConfig& CompilerTools::getConfig(const CodeLanguage inLanguage)
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	chalet_assert(m_configs.find(inLanguage) != m_configs.end(), "CompilerTools::getConfig called before being initialized.");

	auto& config = *m_configs.at(inLanguage);
	return config;
}

/*****************************************************************************/
const CompilerConfig& CompilerTools::getConfig(const CodeLanguage inLanguage) const
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	chalet_assert(m_configs.find(inLanguage) != m_configs.end(), "CompilerTools::getConfig called before being initialized.");

	auto& config = *m_configs.at(inLanguage);
	return config;
}
}
