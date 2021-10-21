/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"

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

	if (toolchainType == ToolchainType::LLVM || toolchainType == ToolchainType::IntelLLVM)
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
	else if (toolchainType == ToolchainType::IntelClassic)
	{
#if defined(CHALET_MACOS)
		auto arch = m_inputs.hostArchitecture();
		m_state.info.setTargetArchitecture(fmt::format("{}-apple-darwin-intel64", arch));
#endif
	}
#if defined(CHALET_WIN32)
	else if (toolchainType == ToolchainType::MSVC)
	{
		if (!detectTargetArchitectureMSVC())
			return false;
	}
#endif

	// Note: Expensive!
	fetchCompilerVersions();

	if (!updateToolchainCacheNode(inConfigJson))
		return false;

	return true;
}

/*****************************************************************************/
#if defined(CHALET_WIN32)
bool CompilerTools::detectTargetArchitectureMSVC()
{
	if (m_msvcArchitectureSet)
		return true;

	std::string host;
	std::string target;

	auto& compiler = !m_compilerCpp.path.empty() ? m_compilerCpp.path : m_compilerC.path;
	std::string lower = String::toLowerCase(compiler);
	auto search = lower.find("/bin/host");
	if (search == std::string::npos)
	{
		Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
		return false;
	}

	auto nextPath = lower.find('/', search + 5);
	if (search == std::string::npos)
	{
		Diagnostic::error("MSVC Host architecture was not detected in compiler path: {}", compiler);
		return false;
	}

	search += 9;
	host = lower.substr(search, nextPath - search);
	search = nextPath + 1;
	nextPath = lower.find('/', search);
	if (search == std::string::npos)
	{
		Diagnostic::error("MSVC Target architecture was not detected in compiler path: {}", compiler);
		return false;
	}

	target = lower.substr(search, nextPath - search);

	m_state.info.setHostArchitecture(host);

	if (host == target)
		m_inputs.setTargetArchitecture(target);
	else
		m_inputs.setTargetArchitecture(fmt::format("{}_{}", host, target));

	m_state.info.setTargetArchitecture(m_inputs.targetArchitecture());

	m_msvcArchitectureSet = true;

	return true;
}
#endif

/*****************************************************************************/
bool CompilerTools::detectToolchainFromPaths()
{
	auto& toolchain = m_inputs.toolchainPreference();
	if (toolchain.type == ToolchainType::Unknown)
	{
#if defined(CHALET_WIN32)
		if (String::endsWith("cl.exe", m_compilerCpp.path) || String::endsWith("cl.exe", m_compilerC.path))
		{
			m_inputs.setToolchainPreferenceType(ToolchainType::MSVC);

			if (!detectTargetArchitectureMSVC())
				return false;
		}
		else
#endif
			if (String::contains("clang", m_compilerCpp.path) || String::contains("clang", m_compilerC.path))
		{
			m_inputs.setToolchainPreferenceType(ToolchainType::LLVM);
		}
		else if (String::contains("icpc", m_compilerCpp.path) || String::contains("icc", m_compilerC.path))
		{
			m_inputs.setToolchainPreferenceType(ToolchainType::IntelClassic);
		}
		else
		{
			// Treat as some variant of GCC
			m_inputs.setToolchainPreferenceType(ToolchainType::GNU);
		}
	}

	return true;
}

/*****************************************************************************/
void CompilerTools::fetchCompilerVersions()
{
	if (m_compilerCpp.description.empty())
	{
		if (!m_compilerCpp.path.empty() && Commands::pathExists(m_compilerCpp.path))
		{
			std::string description;
#if defined(CHALET_WIN32)
			if (m_inputs.toolchainPreference().type == ToolchainType::MSVC)
			{
				description = parseVersionMSVC(m_compilerCpp);
			}
			else
#endif
			{
				description = parseVersionGNU(m_compilerCpp);
			}

			m_compilerCpp.description = std::move(description);
		}
	}

	if (m_compilerC.description.empty())
	{
		auto baseFolderC = String::getPathFolder(m_compilerC.path);
		auto baseFolderCpp = String::getPathFolder(m_compilerCpp.path);
		if (String::equals(baseFolderC, baseFolderCpp))
		{
			m_compilerC.description = m_compilerCpp.description;
		}
		else if (!m_compilerC.path.empty() && Commands::pathExists(m_compilerC.path))
		{
			std::string description;
#if defined(CHALET_WIN32)
			if (m_inputs.toolchainPreference().type == ToolchainType::MSVC)
			{
				description = parseVersionMSVC(m_compilerC);
			}
			else
#endif
			{
				description = parseVersionGNU(m_compilerC);
			}

			m_compilerC.description = std::move(description);
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
bool CompilerTools::updateToolchainCacheNode(JsonFile& inConfigJson)
{
	const auto& settingsFile = m_inputs.settingsFile();
	const auto& preference = m_inputs.toolchainPreferenceName();
	const std::string kKeyOptions{ "options" };
	const std::string kKeyToolchains{ "toolchains" };

	if (!inConfigJson.json.contains(kKeyOptions))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyOptions);
		return false;
	}
	if (!inConfigJson.json.contains(kKeyToolchains))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyToolchains);
		return false;
	}

	auto& toolchains = inConfigJson.json.at(kKeyToolchains);
	if (!toolchains.contains(preference))
	{
		Diagnostic::error("{}: '{}' did not correctly initialize.", settingsFile, kKeyToolchains);
		return false;
	}

	auto& optionsJson = inConfigJson.json.at(kKeyOptions);
	auto& toolchain = toolchains.at(preference);

	const std::string kKeyToolchain{ "toolchain" };
	if (optionsJson.contains(kKeyToolchain))
	{
		auto& toolchainSetting = optionsJson.at(kKeyToolchain);
		if (toolchainSetting.is_string() && toolchainSetting.get<std::string>() != preference)
		{
			optionsJson[kKeyToolchain] = preference;
			inConfigJson.setDirty(true);
		}
	}

	const std::string kKeyArchitecture{ "architecture" };
	if (optionsJson.contains(kKeyArchitecture))
	{
		std::string archString = m_inputs.targetArchitecture().empty() ? "auto" : m_inputs.targetArchitecture();
		auto& arch = optionsJson.at(kKeyArchitecture);
		if (arch.is_string() && arch.get<std::string>() != archString)
		{
			optionsJson[kKeyArchitecture] = archString;
			inConfigJson.setDirty(true);
		}
	}

	if (m_inputs.toolchainPreference().type != ToolchainType::MSVC)
	{
		const std::string kKeyVersion{ "version" };
		if (toolchain.contains(kKeyVersion))
		{
			std::string versionString = m_compilerCpp.version.empty() ? m_compilerC.version : m_compilerCpp.version;
			versionString = versionString.substr(0, versionString.find_first_not_of("0123456789."));

			auto& version = toolchain.at(kKeyVersion);
			if (version.is_string() && version.get<std::string>() != versionString)
			{
				m_version = versionString;
			}
			toolchain[kKeyVersion] = std::move(versionString);
			inConfigJson.setDirty(true);
		}
	}

	return true;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionMSVC(CompilerInfo& outInfo) const
{
	std::string ret;

#if defined(CHALET_WIN32)

	// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
	std::string rawOutput = Commands::subprocessOutput({ outInfo.path });
	auto splitOutput = String::split(rawOutput, '\n');
	if (splitOutput.size() >= 2)
	{
		auto start = splitOutput[1].find("Version");
		auto end = splitOutput[1].find(" for ");
		if (start != std::string::npos && end != std::string::npos)
		{
			start += 8;
			outInfo.version = splitOutput[1].substr(start, end - start); // cl.exe version

			// const auto arch = splitOutput[1].substr(end + 5);
			outInfo.arch = m_state.info.targetArchitectureString();

			// We want the toolchain version as opposed to the cl.exe version (annoying)
			const auto& detectedMsvcVersion = m_state.toolchain.version();
			const auto& toolchainVersion = detectedMsvcVersion.empty() ? m_version : detectedMsvcVersion;

			ret = fmt::format("Microsoft{} Visual C/C++ version {} (VS {})", Unicode::registered(), outInfo.version, toolchainVersion);
		}
	}
#else
	UNUSED(outInfo);
#endif

	return ret;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionGNU(CompilerInfo& outInfo) const
{
	std::string ret;

	// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
	// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
	// Apple clang version 12.0.5 (clang-1205.0.22.9)
	const auto exec = String::getPathBaseName(outInfo.path);
	// const bool isCpp = String::contains("++", exec);
	// const bool isC = String::startsWith({ "gcc", "cc" }, exec);
	std::string rawOutput;
	if (String::contains("clang", outInfo.path))
	{
		rawOutput = Commands::subprocessOutput({ outInfo.path, "-target", m_state.info.targetArchitectureString(), "-v" });
	}
	else if (String::contains({ "icc", "icpc" }, outInfo.path))
	{
		rawOutput = Commands::subprocessOutput({ outInfo.path, "-V" });
	}
	else
	{
		rawOutput = Commands::subprocessOutput({ outInfo.path, "-v" });
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
				outInfo.arch = line.substr(8);
			}
			else if (String::contains("Intel", line))
			{
				compilerRaw = "Intel";
				auto start = line.find("Version ");
				if (start != std::string::npos)
				{
					start += 8;
					auto end = line.find(' ', start);
					versionString = line.substr(start, end - start);
				}
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
				outInfo.version = std::move(versionString);
			}
			else if (String::startsWith("clang", compilerRaw))
			{
				ret = fmt::format("LLVM Clang C/C++ version {}", versionString);
				outInfo.version = std::move(versionString);
			}
#if defined(CHALET_MACOS)
			else if (String::startsWith("Apple clang", compilerRaw))
			{
				ret = fmt::format("Apple Clang C/C++ version {}", versionString);
				outInfo.version = std::move(versionString);
			}
#endif
			else if (String::equals("Intel", compilerRaw))
			{
				ret = fmt::format("Intel{} 64 Compiler Classic version {}", Unicode::registered(), versionString);
				outInfo.version = std::move(versionString);
			}
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
		return m_compilerC.path;
	else
		return m_compilerCpp.path;
}

/*****************************************************************************/
const std::string& CompilerTools::compilerDescriptionStringCpp() const noexcept
{
	return m_compilerCpp.description;
}

const std::string& CompilerTools::compilerDescriptionStringC() const noexcept
{
	return m_compilerC.description;
}

const std::string& CompilerTools::compilerDetectedArchCpp() const noexcept
{
	return m_compilerCpp.arch;
}
const std::string& CompilerTools::compilerDetectedArchC() const noexcept
{
	return m_compilerC.arch;
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
	return m_compilerCpp.path;
}
void CompilerTools::setCompilerCpp(std::string&& inValue) noexcept
{
	m_compilerCpp.path = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::compilerC() const noexcept
{
	return m_compilerC.path;
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

	if (auto ccRoot = String::getPathFolder(m_compilerC.path); !List::contains(pathList, ccRoot))
		outList.emplace_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(m_compilerCpp.path); !List::contains(pathList, cppRoot))
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
