/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/CommandLineInputs.hpp"

#include <thread>

#include "Core/Arch.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CommandLineInputs::CommandLineInputs() :
	m_notPlatforms(getNotPlatforms()),
	kDefaultInputFile("chalet.json"),
	kDefaultSettingsFile(".chaletrc"),
	kDefaultEnvFile(".env"),
	kDefaultOutputDirectory("build"),
	kDefaultExternalDirectory("chalet_external"),
	kDefaultDistributionDirectory("dist"),
	m_settingsFile(kDefaultSettingsFile),
	m_platform(getPlatform()),
	m_hostArchitecture(Arch::getHostCpuArchitecture()),
	m_processorCount(std::thread::hardware_concurrency()),
	m_maxJobs(m_processorCount)
{
	// LOG("Processor count: ", m_processorCount);
}

/*****************************************************************************/
void CommandLineInputs::detectToolchainPreference() const
{
	if (!m_toolchainPreferenceName.empty())
		return;

#if defined(CHALET_WIN32)
	m_toolchainPreference = getToolchainPreferenceFromString("msvc");
#elif defined(CHALET_MACOS)
	m_toolchainPreference = getToolchainPreferenceFromString("apple-llvm");
#else
	m_toolchainPreference = getToolchainPreferenceFromString("gcc");
#endif
}

/*****************************************************************************/
const std::string& CommandLineInputs::workingDirectory() const noexcept
{
	if (m_workingDirectory.empty())
	{
		m_workingDirectory = Commands::getWorkingDirectory();
		Path::sanitize(m_workingDirectory, true);
	}
	return m_workingDirectory;
}

/*****************************************************************************/
const std::string& CommandLineInputs::homeDirectory() const noexcept
{
	if (m_homeDirectory.empty())
	{
		m_homeDirectory = Environment::getUserDirectory();
		Path::sanitize(m_homeDirectory, true);
	}
	return m_homeDirectory;
}

/*****************************************************************************/
const std::string& CommandLineInputs::inputFile() const noexcept
{
	return m_inputFile;
}
void CommandLineInputs::setInputFile(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_inputFile = std::move(inValue);

	Path::sanitize(m_inputFile);
	// clearWorkingDirectory(m_inputFile);
}

/*****************************************************************************/
const std::string& CommandLineInputs::settingsFile() const noexcept
{
	return m_settingsFile;
}
void CommandLineInputs::setSettingsFile(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_settingsFile = std::move(inValue);

	Path::sanitize(m_settingsFile);
	// clearWorkingDirectory(m_settingsFile);
}
const std::string& CommandLineInputs::globalSettingsFile() const noexcept
{
	if (m_globalSettingsFile.empty())
	{
		m_globalSettingsFile = fmt::format("{}/{}", homeDirectory(), kDefaultSettingsFile);
	}
	return m_globalSettingsFile;
}
//

/*****************************************************************************/
const std::string& CommandLineInputs::rootDirectory() const noexcept
{
	return m_rootDirectory;
}

void CommandLineInputs::setRootDirectory(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_rootDirectory = std::move(inValue);
	Path::sanitize(m_rootDirectory);

	if (Commands::pathExists(m_rootDirectory))
	{
		Commands::changeWorkingDirectory(m_rootDirectory);
		m_workingDirectory = Commands::getAbsolutePath(m_rootDirectory);
	}
}

/*****************************************************************************/
const std::string& CommandLineInputs::outputDirectory() const noexcept
{
	return m_outputDirectory;
}

void CommandLineInputs::setOutputDirectory(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_outputDirectory = std::move(inValue);

	Path::sanitize(m_outputDirectory);
	// clearWorkingDirectory(m_outputDirectory);
}

/*****************************************************************************/
const std::string& CommandLineInputs::externalDirectory() const noexcept
{
	return m_externalDirectory;
}

void CommandLineInputs::setExternalDirectory(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_externalDirectory = std::move(inValue);

	Path::sanitize(m_externalDirectory);
	// clearWorkingDirectory(m_externalDirectory);
}

/*****************************************************************************/
const std::string& CommandLineInputs::distributionDirectory() const noexcept
{
	return m_distributionDirectory;
}

void CommandLineInputs::setDistributionDirectory(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_distributionDirectory = std::move(inValue);

	Path::sanitize(m_distributionDirectory);
	// clearWorkingDirectory(m_distributionDirectory);
}
/*****************************************************************************/
const std::string& CommandLineInputs::defaultInputFile() const noexcept
{
	return kDefaultInputFile;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultSettingsFile() const noexcept
{
	return kDefaultSettingsFile;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultEnvFile() const noexcept
{
	return kDefaultEnvFile;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultOutputDirectory() const noexcept
{
	return kDefaultOutputDirectory;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultExternalDirectory() const noexcept
{
	return kDefaultExternalDirectory;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultDistributionDirectory() const noexcept
{
	return kDefaultDistributionDirectory;
}

/*****************************************************************************/
Route CommandLineInputs::command() const noexcept
{
	return m_command;
}

void CommandLineInputs::setCommand(const Route inValue) noexcept
{
	m_command = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildConfiguration() const noexcept
{
	return m_buildConfiguration;
}

void CommandLineInputs::setBuildConfiguration(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::platform() const noexcept
{
	return m_platform;
}
const StringList& CommandLineInputs::notPlatforms() const noexcept
{
	return m_notPlatforms;
}

/*****************************************************************************/
const std::string& CommandLineInputs::runProject() const noexcept
{
	return m_runProject;
}

void CommandLineInputs::setRunProject(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_runProject = std::move(inValue);
}

/*****************************************************************************/
const StringList& CommandLineInputs::runOptions() const noexcept
{
	return m_runOptions;
}

void CommandLineInputs::setRunOptions(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	if (inValue.front() == '\'' && inValue.back() == '\'')
		inValue = inValue.substr(1, inValue.size() - 2);

	// TODO: skip '\ '
	m_runOptions = String::split(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::appPath() const noexcept
{
	return m_appPath;
}

void CommandLineInputs::setAppPath(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_appPath = inValue;
}

/*****************************************************************************/
IdeType CommandLineInputs::generator() const noexcept
{
	return m_generator;
}

const std::string& CommandLineInputs::generatorRaw() const noexcept
{
	return m_generatorRaw;
}

void CommandLineInputs::setGenerator(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_generatorRaw = std::move(inValue);

	m_generator = getIdeTypeFromString(m_generatorRaw);
}

/*****************************************************************************/
const ToolchainPreference& CommandLineInputs::toolchainPreference() const noexcept
{
	return m_toolchainPreference;
}

void CommandLineInputs::setToolchainPreference(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_toolchainPreferenceName = std::move(inValue);

	m_toolchainPreference = getToolchainPreferenceFromString(m_toolchainPreferenceName);
}

/*****************************************************************************/
const std::string& CommandLineInputs::toolchainPreferenceName() const noexcept
{
	return m_toolchainPreferenceName;
}

void CommandLineInputs::setToolchainPreferenceName(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_toolchainPreferenceName = std::move(inValue);
}

/*****************************************************************************/
bool CommandLineInputs::isMsvcPreRelease() const noexcept
{
	return m_isMsvcPreRelease;
}

bool CommandLineInputs::isToolchainPreset() const noexcept
{
	return m_isToolchainPreset;
}

/*****************************************************************************/
const std::string& CommandLineInputs::initPath() const noexcept
{
	return m_initPath;
}

void CommandLineInputs::setInitPath(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_initPath = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::envFile() const noexcept
{
	return m_envFile;
}
void CommandLineInputs::setEnvFile(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_envFile = std::move(inValue);

	Path::sanitize(m_envFile);
	// clearWorkingDirectory(m_envFile);
}

/*****************************************************************************/
const std::string& CommandLineInputs::architectureRaw() const noexcept
{
	return m_architectureRaw;
}
void CommandLineInputs::setArchitectureRaw(std::string&& inValue) const noexcept
{
	m_architectureRaw = std::move(inValue);

	if (String::contains(',', m_architectureRaw))
	{
		auto firstComma = m_architectureRaw.find(',');
		auto arch = m_architectureRaw.substr(0, firstComma);
		auto options = String::split(m_architectureRaw.substr(firstComma + 1), ',');
		setTargetArchitecture(arch);
		setArchOptions(std::move(options));
	}
	else
	{
		setTargetArchitecture(m_architectureRaw);
	}
}

/*****************************************************************************/
const std::string& CommandLineInputs::hostArchitecture() const noexcept
{
	return m_hostArchitecture;
}
const std::string& CommandLineInputs::targetArchitecture() const noexcept
{
	return m_targetArchitecture;
}
void CommandLineInputs::setTargetArchitecture(const std::string& inValue) const noexcept
{
	if (inValue.empty())
		return;

	if (String::equals("auto", inValue))
	{
		m_targetArchitecture.clear();
	}
	else
	{
		m_targetArchitecture = inValue;
	}

#if defined(CHALET_MACOS)
	if (String::equals({ "universal", "universal2" }, m_targetArchitecture))
	{
		m_universalArches = { "x86_64", "arm64" };
	}
	else if (String::equals("universal1", m_targetArchitecture))
	{
		m_universalArches = { "x86_64", "i386" };
	}
#endif
}

/*****************************************************************************/
const StringList& CommandLineInputs::universalArches() const noexcept
{
	return m_universalArches;
}

/*****************************************************************************/
const StringList& CommandLineInputs::archOptions() const noexcept
{
	return m_archOptions;
}

void CommandLineInputs::setArchOptions(StringList&& inList) const noexcept
{
	m_archOptions = std::move(inList);
}

std::string CommandLineInputs::getArchWithOptionsAsString(const std::string& inArchBase) const
{
	std::string ret = inArchBase;
	if (!m_archOptions.empty())
	{
		auto options = String::join(m_archOptions, '_');
		String::replaceAll(options, ',', '_');
		String::replaceAll(options, "-", "");
#if defined(CHALET_WIN32)
		String::replaceAll(options, '=', '_');
#endif

		ret += fmt::format("_{}", options);
	}

	return ret;
}

/*****************************************************************************/
bool CommandLineInputs::saveSchemaToFile() const noexcept
{
	return m_saveSchemaToFile;
}

void CommandLineInputs::setSaveSchemaToFile(const bool inValue) noexcept
{
	m_saveSchemaToFile = inValue;
}

/*****************************************************************************/
SettingsType CommandLineInputs::settingsType() const noexcept
{
	return m_settingsType;
}

void CommandLineInputs::setSettingsType(const SettingsType inValue) noexcept
{
	m_settingsType = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::settingsKey() const noexcept
{
	return m_settingsKey;
}

void CommandLineInputs::setSettingsKey(std::string&& inValue) noexcept
{
	m_settingsKey = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::settingsValue() const noexcept
{
	return m_settingsValue;
}

void CommandLineInputs::setSettingsValue(std::string&& inValue) noexcept
{
	m_settingsValue = std::move(inValue);
}

/*****************************************************************************/
void CommandLineInputs::clearWorkingDirectory(std::string& outValue) const
{
	auto cwd = workingDirectory();
	Path::sanitize(cwd);
	cwd += '/';

	String::replaceAll(outValue, cwd, "");

#if defined(CHALET_WIN32)
	if (::isalpha(cwd.front()) > 0)
	{
		cwd.front() = static_cast<char>(::tolower(cwd.front()));
	}

	String::replaceAll(outValue, cwd, "");
#endif
}

/*****************************************************************************/
uint CommandLineInputs::processorCount() const noexcept
{
	return m_processorCount;
}

/*****************************************************************************/
uint CommandLineInputs::maxJobs() const noexcept
{
	return m_maxJobs;
}

bool CommandLineInputs::maxJobsSetFromCommandLine() const noexcept
{
	return m_maxJobsSetFromCommandLine;
}

void CommandLineInputs::setMaxJobs(const uint inValue, const bool inFromCL) noexcept
{
	m_maxJobs = std::max(inValue, 1U);

	if (inFromCL)
		m_maxJobsSetFromCommandLine = true;
}

/*****************************************************************************/
bool CommandLineInputs::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

bool CommandLineInputs::dumpAssemblySetFromCommandLine() const noexcept
{
	return m_dumpAssemblySetFromCommandLine;
}

void CommandLineInputs::setDumpAssembly(const bool inValue, const bool inFromCL) noexcept
{
	m_dumpAssembly = inValue;

	if (inFromCL)
		m_dumpAssemblySetFromCommandLine = true;
}

/*****************************************************************************/
std::string CommandLineInputs::getPlatform() const noexcept
{
#if defined(CHALET_WIN32)
	return "windows";
#elif defined(CHALET_MACOS)
	return "macos";
#elif defined(CHALET_LINUX)
	return "linux";
#else
	return "unknown";
#endif
}

/*****************************************************************************/
StringList CommandLineInputs::getNotPlatforms() const noexcept
{
#if defined(CHALET_WIN32)
	return {
		"macos", "linux"
	};
#elif defined(CHALET_MACOS)
	return {
		"windows", "linux"
	};
#elif defined(CHALET_LINUX)
	return {
		"windows", "macos"
	};
#else
	return {
		"windows", "macos", "linux"
	};
#endif
}

/*****************************************************************************/
ToolchainPreference CommandLineInputs::getToolchainPreferenceFromString(const std::string& inValue) const
{
	ToolchainPreference ret;
	ret.buildPathStyle = BuildPathStyle::TargetTriple;

	m_isToolchainPreset = false;
	m_isMsvcPreRelease = false;

#if defined(CHALET_WIN32)
	if (String::equals({ "msvc", "msvc-pre" }, inValue))
	{
		m_isToolchainPreset = true;
		m_isMsvcPreRelease = String::equals("msvc-pre", inValue);

		m_toolchainPreferenceName = inValue;

		ret.type = ToolchainType::MSVC;
		ret.strategy = StrategyType::Ninja;
		ret.buildPathStyle = BuildPathStyle::ToolchainName;
		ret.cpp = "cl";
		ret.cc = "cl";
		ret.rc = "rc";
		ret.linker = "link";
		ret.archiver = "lib";
		ret.profiler = ""; // TODO
	}
	else
#endif
#if defined(CHALET_MACOS)
		if (String::equals({ "apple-llvm", "llvm" }, inValue))
#else
	if (String::equals("llvm", inValue))
#endif
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;

		ret.type = ToolchainType::LLVM;
		ret.strategy = StrategyType::Ninja;
		ret.cpp = "clang++";
		ret.cc = "clang";
		ret.rc = "llvm-rc";
		ret.linker = "lld";
		ret.archiver = "ar";
		ret.profiler = ""; // TODO
	}
	else if (String::equals("gcc", inValue))
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;

		ret.type = ToolchainType::GNU;
		ret.strategy = StrategyType::Ninja;
		ret.cpp = "g++";
		ret.cc = "gcc";
		ret.rc = "windres";
		ret.linker = "ld";
		ret.archiver = "ar";
		ret.profiler = "gprof";
	}
	else
	{
		ret.type = ToolchainType::Unknown;
	}

	if (ret.type != ToolchainType::MSVC)
	{
		if (String::equals("x64", m_targetArchitecture))
		{
			setTargetArchitecture("x86_64");
		}
		else if (String::equals("x86", m_targetArchitecture))
		{
			setTargetArchitecture("i686");
		}
	}

	if (Environment::isContinuousIntegrationServer())
	{
		ret.strategy = StrategyType::Native;
	}

	return ret;
}

/*****************************************************************************/
IdeType CommandLineInputs::getIdeTypeFromString(const std::string& inValue) const
{
	if (String::equals("vscode", inValue))
	{
		return IdeType::VisualStudioCode;
	}
	else if (String::equals("vs2019", inValue))
	{
		return IdeType::VisualStudio2019;
	}
	else if (String::equals("xcode", inValue))
	{
		return IdeType::XCode;
	}
	/*else if (String::equals("codeblocks", inValue))
	{
		return IdeType::CodeBlocks;
	}*/
	else if (!inValue.empty())
	{
		return IdeType::Unknown;
	}

	return IdeType::None;
}

}
