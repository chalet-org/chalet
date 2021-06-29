/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/CommandLineInputs.hpp"

#include "Core/Arch.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CommandLineInputs::CommandLineInputs() :
	m_notPlatforms(getNotPlatforms()),
	kDefaultEnvFile(".env"),
	m_buildFile("build.json"),
	m_buildPath("build"),
	m_platform(getPlatform()),
	m_envFile(kDefaultEnvFile),
	m_hostArchitecture(Arch::getHostCpuArchitecture())
{
}

/*****************************************************************************/
void CommandLineInputs::detectToolchainPreference() const
{
	if (!m_toolchainPreferenceRaw.empty())
		return;

#if defined(CHALET_WIN32)
	if (!Commands::which("clang").empty())
		m_toolchainPreference = getToolchainPreferenceFromString("llvm");
	else
		m_toolchainPreference = getToolchainPreferenceFromString("msvc");
#elif defined(CHALET_MACOS)
	m_toolchainPreference = getToolchainPreferenceFromString("llvm");
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
const std::string& CommandLineInputs::buildFile() const noexcept
{
	return m_buildFile;
}
void CommandLineInputs::setBuildFile(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_buildFile = std::move(inValue);

	Path::sanitize(m_buildFile);
	clearWorkingDirectory(m_buildFile);
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildPath() const noexcept
{
	chalet_assert(!m_buildPath.empty(), "buildPath was not defined");
	return m_buildPath;
}
void CommandLineInputs::setBuildPath(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_buildPath = std::move(inValue);

	Path::sanitize(m_buildPath);
	clearWorkingDirectory(m_buildPath);
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultEnvFile() const noexcept
{
	return kDefaultEnvFile;
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

void CommandLineInputs::setBuildConfiguration(const std::string& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildFromCommandLine() const noexcept
{
	return m_buildFromCommandLine;
}
void CommandLineInputs::setBuildFromCommandLine(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_buildFromCommandLine = std::move(inValue);

	setBuildConfiguration(m_buildFromCommandLine);
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

const std::string& CommandLineInputs::toolchainPreferenceRaw() const noexcept
{
	return m_toolchainPreferenceRaw;
}

void CommandLineInputs::setToolchainPreference(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_toolchainPreferenceRaw = std::move(inValue);

	m_toolchainPreference = getToolchainPreferenceFromString(m_toolchainPreferenceRaw);
}

/*****************************************************************************/
bool CommandLineInputs::isMsvcPreRelease() const noexcept
{
	return m_isMsvcPreRelease;
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
	clearWorkingDirectory(m_envFile);
}

/*****************************************************************************/
const std::string& CommandLineInputs::archRaw() const noexcept
{
	return m_archRaw;
}
void CommandLineInputs::setArchRaw(const std::string& inValue) noexcept
{
	m_archRaw = inValue;
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
void CommandLineInputs::setTargetArchitecture(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	if (String::equals("auto", inValue))
	{
		m_targetArchitecture.clear();
	}
	else
	{
		m_targetArchitecture = std::move(inValue);
	}
}

/*****************************************************************************/
const StringList& CommandLineInputs::archOptions() const noexcept
{
	return m_archOptions;
}

void CommandLineInputs::setArchOptions(StringList&& inList) noexcept
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
	auto cwd = workingDirectory() + '/';
	Path::sanitize(cwd);

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

	bool emptyTarget = m_targetArchitecture.empty();
	auto arch = emptyTarget ? m_hostArchitecture : m_targetArchitecture;

#if defined(CHALET_WIN32)
	StringList allowedArchesWin = Arch::getAllowedMsvcArchitectures();
	allowedArchesWin.push_back("x86_64");
	allowedArchesWin.push_back("x64_x64");
	allowedArchesWin.push_back("i686");
	allowedArchesWin.push_back("x86_x86");
	auto splitValue = String::split(inValue, '-');
	StringList msvcSearches{ "msvc", "msvcpre" };
	bool isPlainMSVC = String::equals(msvcSearches, inValue);

	if (isPlainMSVC || (String::equals(msvcSearches, splitValue.front()) && String::equals(allowedArchesWin, splitValue.back())))
	{
		m_isMsvcPreRelease = String::equals("msvcpre", splitValue.front());

		if (String::equals({ "x86_64", "x64_x64" }, arch))
		{
			arch = "x64";
			setTargetArchitecture("x64");
		}
		else if (String::equals({ "i686", "x86_x86" }, arch)) // This implies the host is x86
		{
			arch = "x86";
			setTargetArchitecture("x86");
		}

		if (isPlainMSVC)
			m_toolchainPreferenceRaw = fmt::format("{}-{}", inValue, arch);
		else if (splitValue.size() > 1 && splitValue.back() != arch)
			m_toolchainPreferenceRaw = fmt::format("{}-{}", splitValue.front(), arch);
		else
			m_toolchainPreferenceRaw = inValue;

		ret.type = ToolchainType::MSVC;
		ret.strategy = StrategyType::Ninja;
		ret.cpp = "cl";
		ret.cc = "cl";
		ret.rc = "rc";
		ret.linker = "link";
		ret.archiver = "lib";
		ret.profiler = ""; // TODO
	}
	else
#endif
		if (String::equals("llvm", inValue))
	{
		m_toolchainPreferenceRaw = inValue;

		ret.type = ToolchainType::LLVM;
		ret.strategy = StrategyType::Makefile;
		ret.cpp = "clang++";
		ret.cc = "clang";
		ret.rc = "llvm-rc";
		ret.linker = "lld";
		ret.archiver = "ar";
		ret.profiler = ""; // TODO
	}
	else if (String::equals("gcc", inValue))
	{
		m_toolchainPreferenceRaw = inValue;

		ret.type = ToolchainType::GNU;
		ret.strategy = StrategyType::Makefile;
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
