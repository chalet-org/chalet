/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "State/CompilerTools.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerTools::CompilerTools(const CommandLineInputs& inInputs, const BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState)
{
	m_detectedToolchain = m_inputs.toolchainPreference().type;
}

/*****************************************************************************/
void CompilerTools::detectToolchain()
{
#if defined(CHALET_WIN32)
	if (String::endsWith("cl.exe", m_cc) || String::endsWith("cl.exe", m_cpp))
	{
		m_detectedToolchain = ToolchainType::MSVC;
	}
	else
#endif
		if (String::contains("clang", m_cc) || String::contains("clang", m_cpp))
	{
		m_detectedToolchain = ToolchainType::LLVM;
	}
	else if (String::contains("gcc", m_cc) || String::contains("g++", m_cpp))
	{
		m_detectedToolchain = ToolchainType::GNU;
	}
	else
	{
		m_detectedToolchain = ToolchainType::Unknown;
	}

	m_ccDetected = Commands::pathExists(m_cc);
}

/*****************************************************************************/
bool CompilerTools::initialize(const BuildTargetList& inTargets)
{
	fetchCompilerVersions();

	// Note: Expensive!
	if (!initializeCompilerConfigs(inTargets))
		return false;

#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalArm64_X64)
		return true;
#endif

	if (m_detectedToolchain == ToolchainType::LLVM)
	{
		auto results = Commands::subprocessOutput({ compiler(), "-print-targets" });
		if (!String::contains("error:", results))
		{
			auto split = String::split(results, String::eol());
			bool valid = false;
			// m_state.info.setTargetArchitecture(arch);
			const auto& targetArch = m_state.info.targetArchitectureString();
			for (auto& line : split)
			{
				auto start = line.find_first_not_of(' ');
				auto end = line.find_first_of(' ', start);

				auto arch = line.substr(start, end - start);
				if (String::startsWith(arch, targetArch))
					valid = true;
			}

			UNUSED(split);
			return valid;
		}
	}
	else if (m_detectedToolchain == ToolchainType::GNU)
	{
		const auto& arch = m_inputs.targetArchitecture();
		if (!arch.empty())
		{
			const auto& targetArch = m_state.info.targetArchitectureString();

			return String::startsWith(arch, targetArch);
		}
	}
#if defined(CHALET_WIN32)
	else if (m_detectedToolchain == ToolchainType::MSVC)
	{
		const auto& targetArch = m_state.info.targetArchitectureString();
		auto arch = String::getPathFilename(String::getPathFolder(compiler()));

		if (String::equals("x64", arch))
			arch = "x86_64";
		else if (String::equals("x86", arch))
			arch = "i686";

		return String::startsWith(arch, targetArch);
	}
#endif

	return true;
}

/*****************************************************************************/
void CompilerTools::fetchCompilerVersions()
{
	if (m_compilerVersionStringCpp.empty())
	{
		if (!m_cpp.empty() && Commands::pathExists(m_cpp))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_detectedToolchain == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_cpp);
			}
			else
			{
				version = parseVersionGNU(m_cpp, String::eol());
			}
#else
			version = parseVersionGNU(m_cpp);
#endif
			m_compilerVersionStringCpp = std::move(version);
		}
	}

	if (m_compilerVersionStringC.empty())
	{
		if (!m_cc.empty() && Commands::pathExists(m_cc))
		{
			std::string version;
#if defined(CHALET_WIN32)
			if (m_detectedToolchain == ToolchainType::MSVC)
			{
				version = parseVersionMSVC(m_cc);
			}
			else
			{
				version = parseVersionGNU(m_cc, String::eol());
			}
#else
			version = parseVersionGNU(m_cc);
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
			Diagnostic::error(fmt::format("Error collecting supported compiler flags for '{}'.", exec));
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionMSVC(const std::string& inExecutable) const
{
	std::string ret;

	// Microsoft (R) C/C++ Optimizing Compiler Version 19.28.29914 for x64
	std::string rawOutput = Commands::subprocessOutput({ inExecutable });
	auto splitOutput = String::split(rawOutput, String::eol());
	if (splitOutput.size() >= 2)
	{
		auto start = splitOutput[1].find("Version");
		auto end = splitOutput[1].find(" for ");
		if (start != std::string::npos && end != std::string::npos)
		{
			const auto versionString = splitOutput[1].substr(start, end - start);
			// const auto arch = splitOutput[1].substr(end + 5);
			const auto arch = m_state.info.targetArchitectureString();
			ret = fmt::format("Microsoft{} Visual C/C++ {} [{}]", Unicode::registered(), versionString, arch);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string CompilerTools::parseVersionGNU(const std::string& inExecutable, const std::string_view inEol) const
{
	std::string ret;

	// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
	// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
	// Apple clang version 12.0.5 (clang-1205.0.22.9)
	const auto exec = String::getPathBaseName(inExecutable);
	const bool isCpp = String::contains("++", exec);
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

	auto splitOutput = String::split(rawOutput, inEol);
	if (splitOutput.size() >= 2)
	{
		std::string versionString;
		std::string compilerRaw;
		std::string arch;
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
				arch = line.substr(8);
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
				ret = fmt::format("GNU Compiler Collection {} Version {} [{}]", isCpp ? "C++" : "C", versionString, arch);
			}
			else if (String::startsWith("Apple clang", compilerRaw))
			{
				ret = fmt::format("Apple Clang {} Version {} [{}]", isCpp ? "C++" : "C", versionString, arch);
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
			if (vals.size() == 3)
			{
				m_ninjaVersionMajor = std::stoi(vals[0]);
				m_ninjaVersionMinor = std::stoi(vals[1]);
				m_ninjaVersionPatch = std::stoi(vals[2]);
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

void CompilerTools::setStrategy(const std::string& inValue) noexcept
{
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
		chalet_assert(false, "Invalid strategy type");
	}
}

/*****************************************************************************/
ToolchainType CompilerTools::detectedToolchain() const
{
	return m_detectedToolchain;
}

/*****************************************************************************/
const std::string& CompilerTools::compiler() const noexcept
{
	if (m_ccDetected)
		return m_cc;
	else
		return m_cpp;
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
const std::string& CompilerTools::cpp() const noexcept
{
	return m_cpp;
}
void CompilerTools::setCpp(std::string&& inValue) noexcept
{
	m_cpp = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::cc() const noexcept
{
	return m_cc;
}
void CompilerTools::setCc(std::string&& inValue) noexcept
{
	m_cc = std::move(inValue);
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
const std::string& CompilerTools::objdump() const noexcept
{
	return m_objdump;
}
void CompilerTools::setObjdump(std::string&& inValue) noexcept
{
	m_objdump = std::move(inValue);
}

/*****************************************************************************/
const std::string& CompilerTools::rc() const noexcept
{
	return m_rc;
}
void CompilerTools::setRc(std::string&& inValue) noexcept
{
	m_rc = std::move(inValue);
}

/*****************************************************************************/
std::string CompilerTools::getRootPathVariable()
{
	auto originalPath = Environment::getPath();
	Path::sanitize(originalPath);

	StringList outList;

	if (auto ccRoot = String::getPathFolder(m_cc); !List::contains(outList, ccRoot))
		outList.push_back(std::move(ccRoot));

	if (auto cppRoot = String::getPathFolder(m_cpp); !List::contains(outList, cppRoot))
		outList.push_back(std::move(cppRoot));

	for (auto& p : Path::getOSPaths())
	{
		if (!Commands::pathExists(p))
			continue;

		auto path = Commands::getCanonicalPath(p); // probably not needed, but just in case

		List::addIfDoesNotExist(outList, std::move(path));
	}

	char separator = Path::getSeparator();
	for (auto& path : String::split(originalPath, separator))
	{
		List::addIfDoesNotExist(outList, std::move(path));
	}

	std::string ret = String::join(outList, separator);
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
