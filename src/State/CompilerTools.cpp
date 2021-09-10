/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"

#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
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
CompilerTools::CompilerTools(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState)
{
}

/*****************************************************************************/
bool CompilerTools::initialize(const BuildTargetList& inTargets, JsonFile& inConfigJson)
{
	ToolchainType toolchainType = m_inputs.toolchainPreference().type;
	Arch::Cpu targetArch = m_state.info.targetArchitecture();

	if (!initializeCompilerConfigs(inTargets))
	{
		Diagnostic::error("Compiler was not recognized.");
		return false;
	}

	const auto& archFromInput = m_inputs.targetArchitecture();
	const auto& targetArchString = m_state.info.targetArchitectureString();

	if (toolchainType == ToolchainType::LLVM)
	{
		bool valid = false;
		if (!archFromInput.empty())
		{
			auto results = Commands::subprocessOutput({ compilerCxx(), "-print-targets" });
			if (!String::contains("error:", results))
			{
				auto split = String::split(results, "\n");
				for (auto& line : split)
				{
					auto start = line.find_first_not_of(' ');
					auto end = line.find_first_of(' ', start);

					auto result = line.substr(start, end - start);
					if (targetArch == Arch::Cpu::X64)
					{
						if (String::equals({ "x86-64", "x86_64", "x64" }, result))
							valid = true;
					}
					else if (targetArch == Arch::Cpu::X86)
					{
						if (String::equals({ "i686", "x86" }, result))
							valid = true;
					}
					else
					{
						if (String::startsWith(result, targetArchString))
							valid = true;
					}
				}
			}
		}

		if (!String::contains('-', targetArchString))
		{
			auto result = Commands::subprocessOutput({ compilerCxx(), "-dumpmachine" });
			auto firstDash = result.find_first_of('-');
			if (!result.empty() && firstDash != std::string::npos)
			{
				result = fmt::format("{}{}", targetArchString, result.substr(firstDash));
#if defined(CHALET_MACOS)
				// Strip out version in auto-detected mac triple
				auto darwin = result.find("apple-darwin");
				if (darwin != std::string::npos)
				{
					result = result.substr(0, darwin + 12);
				}
#endif
				m_state.info.setTargetArchitecture(result);
				valid = true;
			}
		}
		else
		{
			valid = true;
		}

		if (!valid)
			return false;
	}
	else if (toolchainType == ToolchainType::GNU)
	{
		if (archFromInput.empty() || !String::contains('-', targetArchString))
		{
			auto result = Commands::subprocessOutput({ compilerCxx(), "-dumpmachine" });
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto darwin = result.find("apple-darwin");
			if (darwin != std::string::npos)
			{
				result = result.substr(0, darwin + 12);
			}
#endif
			m_state.info.setTargetArchitecture(result);
		}
		else
		{
			// Pass along and hope for the best

			// if (!String::startsWith(archFromInput, targetArchString))
			// 	return false;
		}
	}
#if defined(CHALET_WIN32)
	else if (toolchainType == ToolchainType::MSVC)
	{
		auto allowedArches = Arch::getAllowedMsvcArchitectures();
		if (!String::equals(allowedArches, targetArchString))
			return false;

		std::string host;
		std::string target = targetArchString;
		if (String::contains('_', target))
		{
			auto split = String::split(target, '_');
			host = split.front();
			target = split.back();
		}
		else
		{
			host = target;
		}

		auto& compiler = !m_compilerCpp.empty() ? m_compilerCpp : m_compilerC;
		std::string lower = String::toLowerCase(compiler);
		auto search = lower.find(fmt::format("/host{}/{}/", host, target));
		if (search == std::string::npos)
		{
			Diagnostic::error("Target architecture '{}' was not found in compiler path: {}", target, compiler);
			return false;
		}

		m_state.info.setHostArchitecture(host);
		m_state.info.setTargetArchitecture(fmt::format("{}-pc-windows-msvc", targetArchString));
	}
#endif

	if (!updateToolchainCacheNode(inConfigJson))
		return false;

	// Note: Expensive!
	fetchCompilerVersions();

	return true;
}

/*****************************************************************************/
void CompilerTools::detectToolchainFromPaths()
{
	auto& toolchain = m_inputs.toolchainPreference();
	if (toolchain.type == ToolchainType::Unknown)
	{
#if defined(CHALET_WIN32)
		if (String::endsWith("cl.exe", m_compilerCpp) || String::endsWith("cl.exe", m_compilerC))
		{
			toolchain.setType(ToolchainType::MSVC);
		}
		else
#endif
			if (String::contains("clang", m_compilerCpp) || String::contains("clang", m_compilerC))
		{
			toolchain.setType(ToolchainType::LLVM);
		}
		else
		{
			// Treat as some variant of GCC
			toolchain.setType(ToolchainType::GNU);
		}
	}
}

/*****************************************************************************/
void CompilerTools::fetchCompilerVersions()
{
	if (m_compilerVersionStringCpp.empty())
	{
		if (!m_compilerCpp.empty() && Commands::pathExists(m_compilerCpp))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_inputs.toolchainPreference().type == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_compilerCpp, m_compilerDetectedArchCpp);
			}
			else
			{
				version = parseVersionGNU(m_compilerCpp, m_compilerDetectedArchCpp);
			}
#else
			version = parseVersionGNU(m_compilerCpp, m_compilerDetectedArchCpp);
#endif
			m_compilerVersionStringCpp = std::move(version);
		}
	}

	if (m_compilerVersionStringC.empty())
	{
		if (!m_compilerC.empty() && Commands::pathExists(m_compilerC))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_inputs.toolchainPreference().type == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_compilerC, m_compilerDetectedArchC);
			}
			else
			{
				version = parseVersionGNU(m_compilerC, m_compilerDetectedArchC);
			}
#else
			version = parseVersionGNU(m_compilerC, m_compilerDetectedArchC);
#endif
			m_compilerVersionStringC = std::move(version);
		}
	}
}

/*****************************************************************************/
bool CompilerTools::initializeCompilerConfigs(const BuildTargetList& inTargets)
{
	for (auto& target : inTargets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<ProjectTarget&>(*target);
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
bool CompilerTools::updateToolchainCacheNode(JsonFile& inConfigJson)
{
	const auto& preference = m_inputs.toolchainPreferenceName();
	auto& buildSettings = inConfigJson.json["settings"];

	buildSettings["toolchain"] = preference;
	inConfigJson.setDirty(true);

	return true;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionMSVC(const std::string& inExecutable, std::string& outArch) const
{
	std::string ret;

#if defined(CHALET_WIN32)

	// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
	std::string rawOutput = Commands::subprocessOutput({ inExecutable });
	auto splitOutput = String::split(rawOutput, '\n');
	if (splitOutput.size() >= 2)
	{
		auto start = splitOutput[1].find("Version");
		auto end = splitOutput[1].find(" for ");
		if (start != std::string::npos && end != std::string::npos)
		{
			start += 8;
			const auto clVersion = splitOutput[1].substr(start, end - start); // cl.exe version

			// const auto arch = splitOutput[1].substr(end + 5);
			outArch = m_state.info.targetArchitectureString();

			// We want the toolchain version as opposed to the cl.exe version (annoying)
			const auto& detectedMsvcVersion = m_state.msvcEnvironment.detectedVersion();
			const auto& toolchainVersion = detectedMsvcVersion.empty() ? m_version : detectedMsvcVersion;

			ret = fmt::format("Microsoft{} Visual C/C++ version {} (cl-{})", Unicode::registered(), toolchainVersion, clVersion);
		}
	}
#else
	UNUSED(inExecutable, outArch);
#endif

	return ret;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionGNU(const std::string& inExecutable, std::string& outArch) const
{
	std::string ret;

	// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
	// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
	// Apple clang version 12.0.5 (clang-1205.0.22.9)
	const auto exec = String::getPathBaseName(inExecutable);
	// const bool isCpp = String::contains("++", exec);
	// const bool isC = String::startsWith({ "gcc", "cc" }, exec);
	std::string rawOutput;
	if (String::contains("clang", inExecutable))
	{
		rawOutput = Commands::subprocessOutput({ inExecutable, "-target", m_state.info.targetArchitectureString(), "-v" });
	}
	else
	{
		rawOutput = Commands::subprocessOutput({ inExecutable, "-v" });
	}

	auto splitOutput = String::split(rawOutput, '\n');
	if (splitOutput.size() >= 2)
	{
		std::string versionString;
		std::string compilerRaw;
		std::string threadModel;
		for (auto& line : splitOutput)
		{
			if (String::contains("version", line))
			{
				auto start = line.find("version");
				compilerRaw = line.substr(0, start - 1);
				versionString = line.substr(start + 8);

				while (versionString.back() == ' ')
					versionString.pop_back();
			}
			else if (String::startsWith("Target:", line))
			{
				outArch = line.substr(8);
			}
			/*else if (String::startsWith("Thread model:", line))
			{
				threadModel = line.substr(14);
			}*/
			// Supported LTO compression algorithms:
		}
		UNUSED(threadModel);

		if (!compilerRaw.empty())
		{
			if (String::startsWith("gcc", compilerRaw))
			{
				ret = fmt::format("GNU Compiler Collection C/C++ version {}", versionString);
				m_version = versionString;
			}
			else if (String::startsWith("clang", compilerRaw))
			{
				ret = fmt::format("LLVM Clang C/C++ version {}", versionString);
				m_version = versionString;
			}
#if defined(CHALET_MACOS)
			else if (String::startsWith("Apple clang", compilerRaw))
			{
				ret = fmt::format("Apple Clang C/C++ version {}", versionString);
				m_version = versionString;
			}
#endif
		}
		else
		{
			ret = "Unrecognized";
		}
	}

	return ret;
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
	else if (String::equals(inValue, "native-experimental"))
	{
		m_strategy = StrategyType::Native;
	}
	else if (String::equals(inValue, "ninja"))
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
const std::string& CompilerTools::compilerVersionStringCpp() const noexcept
{
	return m_compilerVersionStringCpp;
}

const std::string& CompilerTools::compilerVersionStringC() const noexcept
{
	return m_compilerVersionStringC;
}

const std::string& CompilerTools::compilerDetectedArchCpp() const noexcept
{
	return m_compilerDetectedArchCpp;
}
const std::string& CompilerTools::compilerDetectedArchC() const noexcept
{
	return m_compilerDetectedArchC;
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
std::string CompilerTools::getRootPathVariable()
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
}

/*****************************************************************************/
CompilerConfig& CompilerTools::getConfig(const CodeLanguage inLanguage) const
{
	chalet_assert(inLanguage != CodeLanguage::None, "Invalid language requested.");
	chalet_assert(m_configs.find(inLanguage) != m_configs.end(), "CompilerTools::getConfig called before being initialized.");

	auto& config = *m_configs.at(inLanguage);
	return config;
}
}
