/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/CommandLineInputs.hpp"

#include "Core/Arch.hpp"
#include "Core/Platform.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
Dictionary<InitTemplateType> getInitTemplates()
{
	return {
		{ "cmake", InitTemplateType::CMake },
	};
}

OrderedDictionary<ExportKind> getExportKinds()
{
	return {
		{ "vscode", ExportKind::VisualStudioCodeJSON },
		{ "vsjson", ExportKind::VisualStudioJSON },
		{ "vssolution", ExportKind::VisualStudioSolution },
		{ "codeblocks", ExportKind::CodeBlocks },
	};
}

Dictionary<QueryOption> getQueryOptions()
{
	return {
		{ "all-toolchains", QueryOption::AllToolchains },
		{ "architecture", QueryOption::Architecture },
		{ "architectures", QueryOption::Architectures },
		{ "options", QueryOption::Options },
		{ "commands", QueryOption::Commands },
		{ "configuration", QueryOption::Configuration },
		{ "configurations", QueryOption::Configurations },
		{ "list-names", QueryOption::QueryNames },
		{ "export-kinds", QueryOption::ExportKinds },
		{ "run-target", QueryOption::RunTarget },
		{ "all-build-targets", QueryOption::AllBuildTargets },
		{ "all-run-targets", QueryOption::AllRunTargets },
		{ "theme-names", QueryOption::ThemeNames },
		{ "toolchain", QueryOption::Toolchain },
		{ "toolchain-presets", QueryOption::ToolchainPresets },
		{ "user-toolchains", QueryOption::UserToolchains },
		{ "build-strategy", QueryOption::BuildStrategy },
		{ "build-strategies", QueryOption::BuildStrategies },
		{ "build-path-style", QueryOption::BuildPathStyle },
		{ "build-path-styles", QueryOption::BuildPathStyles },
		{ "state-chalet-json", QueryOption::ChaletJsonState },
		{ "state-settings-json", QueryOption::SettingsJsonState },
		{ "schema-chalet-json", QueryOption::ChaletSchema },
		{ "schema-settings-json", QueryOption::SettingsSchema },
		{ "version", QueryOption::Version },
	};
}

#if defined(CHALET_WIN32)
OrderedDictionary<VisualStudioVersion> getVisualStudioPresets()
{
	return {
		// { "vs-2010", VisualStudioVersion::VisualStudio2010 }, // untested
		// { "vs-2012", VisualStudioVersion::VisualStudio2012 }, // untested
		// { "vs-2013", VisualStudioVersion::VisualStudio2013 }, // untested
		// { "vs-2015", VisualStudioVersion::VisualStudio2015 }, // untested
		{ "vs-2017", VisualStudioVersion::VisualStudio2017 },
		{ "vs-2019", VisualStudioVersion::VisualStudio2019 },
		{ "vs-2022", VisualStudioVersion::VisualStudio2022 },
		{ "vs-preview", VisualStudioVersion::Preview },
		{ "vs-stable", VisualStudioVersion::Stable },
	};
}
OrderedDictionary<VisualStudioVersion> getVisualStudioLLVMPresets()
{
	return {
		{ "llvm-vs-2019", VisualStudioVersion::VisualStudio2019 },
		{ "llvm-vs-2022", VisualStudioVersion::VisualStudio2022 },
	};
}
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
OrderedDictionary<VisualStudioVersion> getIntelClassicVSPresets()
{
	return {
		{ "intel-classic-vs-2017", VisualStudioVersion::VisualStudio2017 },
		// { "intel-classic-vs-2019", VisualStudioVersion::VisualStudio2019 },
		// { "intel-classic-vs-2022", VisualStudioVersion::VisualStudio2022 },
	};
}
	#endif
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
OrderedDictionary<VisualStudioVersion> getIntelClangVSPresets()
{
	return {
		// { "intel-llvm-vs-2017", VisualStudioVersion::VisualStudio2017 },
		{ "intel-llvm-vs-2019", VisualStudioVersion::VisualStudio2019 },
		{ "intel-llvm-vs-2022", VisualStudioVersion::VisualStudio2022 },
	};
}
	#endif
#endif
}

/*****************************************************************************/
CommandLineInputs::CommandLineInputs() :
	kDefaultInputFile("chalet.json"),
	kDefaultSettingsFile(".chaletrc"),
	kDefaultEnvFile(".env"),
	kDefaultOutputDirectory("build"),
	kDefaultExternalDirectory("chalet_external"),
	kDefaultDistributionDirectory("dist"),
	kGlobalSettingsFile(".chalet/config.json"),
	kArchPresetAuto("auto"),
	kToolchainPresetGCC("gcc"),
	kToolchainPresetLLVM("llvm"),
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC && !defined(CHALET_WIN32)
	kToolchainPresetICC("intel-classic"),
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	#if !defined(CHALET_WIN32)
	kToolchainPresetICX("intel-llvm"),
	#endif
#endif
#if defined(CHALET_WIN32)
	kToolchainPresetVisualStudioStable("vs-stable"),
#elif defined(CHALET_MACOS)
	kToolchainPresetAppleLLVM("apple-llvm"),
#endif
	kBuildStrategyNinja("ninja"),
	m_settingsFile(kDefaultSettingsFile),
	m_hostArchitecture(Arch::getHostCpuArchitecture())
{
	// LOG("Processor count: ", m_processorCount);
}

/*****************************************************************************/
void CommandLineInputs::detectToolchainPreference() const
{
	if (m_toolchainPreferenceName.empty())
	{
		const auto& defaultPreset = defaultToolchainPreset();
		m_toolchainPreference = getToolchainPreferenceFromString(defaultPreset);
	}
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultArchPreset() const noexcept
{
	return kArchPresetAuto;
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultToolchainPreset() const noexcept
{
#if defined(CHALET_WIN32)
	return kToolchainPresetVisualStudioStable;
#elif defined(CHALET_MACOS)
	return kToolchainPresetAppleLLVM;
#else
	return kToolchainPresetGCC;
#endif
}

/*****************************************************************************/
const std::string& CommandLineInputs::defaultBuildStrategy() const noexcept
{
	return kBuildStrategyNinja;
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
const std::string& CommandLineInputs::globalSettingsFile() const noexcept
{
	return kGlobalSettingsFile;
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

std::string CommandLineInputs::getGlobalSettingsFilePath() const
{
	return fmt::format("{}/{}", homeDirectory(), kGlobalSettingsFile);
}

std::string CommandLineInputs::getGlobalDirectory() const
{
	return fmt::format("{}/.chalet", homeDirectory());
}

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
const CommandRoute& CommandLineInputs::route() const noexcept
{
	return m_route;
}

void CommandLineInputs::setRoute(const CommandRoute& inValue) noexcept
{
	m_route = inValue;
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

	m_buildConfiguration = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::lastTarget() const noexcept
{
	return m_lastTarget;
}

void CommandLineInputs::setLastTarget(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_lastTarget = std::move(inValue);
}

/*****************************************************************************/
const std::optional<StringList>& CommandLineInputs::runArguments() const noexcept
{
	return m_runArguments;
}

void CommandLineInputs::setRunArguments(StringList&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_runArguments = std::move(inValue);
}

void CommandLineInputs::setRunArguments(const StringList& inValue) const noexcept
{
	if (inValue.empty())
		return;

	m_runArguments = inValue;
}

void CommandLineInputs::setRunArguments(std::string&& inValue) const noexcept
{
	if (inValue.empty())
		return;

	if (inValue.front() == '\'' && inValue.back() == '\'')
		inValue = inValue.substr(1, inValue.size() - 2);

	// TODO: skip '\ '
	m_runArguments = String::split(inValue);
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
ExportKind CommandLineInputs::exportKind() const noexcept
{
	return m_exportKind;
}

const std::string& CommandLineInputs::exportKindRaw() const noexcept
{
	return m_exportKindRaw;
}

void CommandLineInputs::setExportKind(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_exportKindRaw = std::move(inValue);

	m_exportKind = getExportKindFromString(m_exportKindRaw);
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

void CommandLineInputs::setToolchainPreferenceType(const ToolchainType inValue) const noexcept
{
	m_toolchainPreference.setType(inValue);
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
const std::string& CommandLineInputs::buildStrategyPreference() const noexcept
{
	return m_buildStrategyPreference;
}

void CommandLineInputs::setBuildStrategyPreference(std::string&& inValue)
{
	m_buildStrategyPreference = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::buildPathStylePreference() const noexcept
{
	return m_buildPathStylePreference;
}

void CommandLineInputs::setBuildPathStylePreference(std::string&& inValue)
{
	m_buildPathStylePreference = std::move(inValue);
}

/*****************************************************************************/
VisualStudioVersion CommandLineInputs::visualStudioVersion() const noexcept
{
	return m_visualStudioVersion;
}

bool CommandLineInputs::isToolchainPreset() const noexcept
{
	return m_isToolchainPreset;
}

bool CommandLineInputs::isToolchainMultiArchPreset() const noexcept
{
	return m_isToolchainMultiArchPreset;
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
InitTemplateType CommandLineInputs::initTemplate() const noexcept
{
	return m_initTemplate;
}

void CommandLineInputs::setInitTemplate(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_initTemplate = getInitTemplateFromString(inValue);
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
std::string CommandLineInputs::platformEnv() const noexcept
{
	const auto& platform = Platform::platform();
	return fmt::format("{}.{}", kDefaultEnvFile, platform);
}

/*****************************************************************************/
void CommandLineInputs::resolveEnvFile()
{
	auto searchDotEnv = [this](const std::string& inRelativeEnv, const std::string& inEnv) {
		if (String::endsWith(kDefaultEnvFile, inRelativeEnv))
		{
			auto toSearch = String::getPathFolder(inRelativeEnv);
			if (!toSearch.empty())
			{
				toSearch = fmt::format("{}/{}", toSearch, inEnv);
				if (Commands::pathExists(toSearch))
					return toSearch;
			}
		}
		else
		{
			if (Commands::pathExists(inRelativeEnv))
				return inRelativeEnv;
		}

		if (Commands::pathExists(inEnv))
			return inEnv;

		return std::string();
	};

	std::string tmp = searchDotEnv(m_envFile, platformEnv());
	if (tmp.empty())
	{
		tmp = searchDotEnv(m_envFile, kDefaultEnvFile);
	}

	if (tmp.empty())
	{
		if (Commands::pathExists(m_envFile))
			tmp = m_envFile;
	}

	if (!tmp.empty())
	{
		setEnvFile(std::move(tmp));
	}
}

/*****************************************************************************/
const std::string& CommandLineInputs::architectureRaw() const noexcept
{
	return m_architectureRaw;
}
void CommandLineInputs::setArchitectureRaw(std::string&& inValue) const noexcept
{
	// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
	// Either parsed later (if MSVC) or passed directly to GNU compiler
	if (std::all_of(inValue.begin(), inValue.end(), [](char c) {
			return ::isalpha(c)
				|| ::isdigit(c)
				|| c == '-'
				|| c == ','
				|| c == '.'
#if defined(CHALET_WIN32)
				|| c == '='
#endif
				|| c == '_';
		}))
	{
		m_architectureRaw = std::move(inValue);
	}
	else
	{
		m_architectureRaw = std::string{ "auto" };
	}

	if (String::contains(',', m_architectureRaw))
	{
		auto firstComma = m_architectureRaw.find(',');
		auto arch = m_architectureRaw.substr(0, firstComma);
		auto options = String::split(m_architectureRaw.substr(firstComma + 1), ',');
		setTargetArchitecture(arch);
		// setArchOptions(std::move(options));
		UNUSED(options);
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
std::string CommandLineInputs::getResolvedTargetArchitecture() const noexcept
{
	if (m_targetArchitecture.empty())
		return m_hostArchitecture;

	return m_targetArchitecture;
}

void CommandLineInputs::setTargetArchitecture(const std::string& inValue) const noexcept
{
	if (inValue.empty())
		return;

	if (String::equals(kArchPresetAuto, inValue))
	{
		m_targetArchitecture.clear();
	}
	else
	{
#if defined(CHALET_MACOS)
		if (String::equals(StringList{ "universal", "universal2" }, inValue))
		{
			m_universalArches = { "x86_64", "arm64" };
		}
		else if (String::equals("universal1", inValue))
		{
			m_universalArches = { "x86_64", "i386" };
		}
#endif

		// This will convert the input into a GNU-compatible arch
		m_targetArchitecture = Arch::toGnuArch(inValue);
	}
}

/*****************************************************************************/
const std::string& CommandLineInputs::signingIdentity() const noexcept
{
	return m_signingIdentity;
}
void CommandLineInputs::setSigningIdentity(std::string&& inValue) noexcept
{
	m_signingIdentity = std::move(inValue);
}

/*****************************************************************************/
const std::string& CommandLineInputs::osTargetName() const noexcept
{
	return m_osTargetName;
}
void CommandLineInputs::setOsTargetName(std::string&& inValue) noexcept
{
	m_osTargetName = std::move(inValue);
}
std::string CommandLineInputs::getDefaultOsTargetName() const
{
#if defined(CHALET_MACOS)
	return std::string("macosx");
#else
	return std::string();
#endif
}

/*****************************************************************************/
const std::string& CommandLineInputs::osTargetVersion() const noexcept
{
	return m_osTargetVersion;
}
void CommandLineInputs::setOsTargetVersion(std::string&& inValue) noexcept
{
	m_osTargetVersion = std::move(inValue);
}
std::string CommandLineInputs::getDefaultOsTargetVersion() const
{
#if defined(CHALET_MACOS)
	if (kDefaultOsTarget.empty())
	{
		auto swVers = Commands::which("sw_vers");
		if (!swVers.empty())
		{
			auto result = Commands::subprocessOutput({ swVers });
			if (!result.empty())
			{
				auto split = String::split(result, '\n');
				for (auto& line : split)
				{
					// Note: there is also "ProductName" but it varies between os versions
					//  - Older versions had "Mac OS X" and newer ones have "macOS"
					//
					if (String::startsWith("ProductVersion", line))
					{
						auto lastTab = line.find_last_of('\t');
						if (lastTab != std::string::npos)
						{
							auto value = line.substr(lastTab + 1);
							kDefaultOsTarget = String::toLowerCase(value);
						}
					}
				}
			}
		}
	}
	return kDefaultOsTarget;
#else
	return std::string();
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
		String::replaceAll(options, '-', "");
#if defined(CHALET_WIN32)
		String::replaceAll(options, '=', '_');
#endif

		ret += fmt::format("_{}", options);
	}

	return ret;
}

/*****************************************************************************/
const StringList& CommandLineInputs::commandList() const noexcept
{
	return m_commandList;
}
void CommandLineInputs::setCommandList(StringList&& inList) noexcept
{
	m_commandList = std::move(inList);
}

/*****************************************************************************/
const StringList& CommandLineInputs::queryData() const noexcept
{
	return m_queryData;
}
void CommandLineInputs::setQueryData(StringList&& inList) noexcept
{
	m_queryData = std::move(inList);
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
QueryOption CommandLineInputs::queryOption() const noexcept
{
	return m_queryOption;
}
void CommandLineInputs::setQueryOption(std::string&& inValue) noexcept
{
	m_queryOption = getQueryOptionFromString(inValue);
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

	String::replaceAll(outValue, cwd, "");

#if defined(CHALET_WIN32)
	if (::isalpha(cwd.front()) > 0)
	{
		cwd[0] = static_cast<char>(::tolower(static_cast<uchar>(cwd.front())));
	}

	String::replaceAll(outValue, cwd, "");
#endif
}

/*****************************************************************************/
const std::optional<uint>& CommandLineInputs::maxJobs() const noexcept
{
	return m_maxJobs;
}

void CommandLineInputs::setMaxJobs(const uint inValue) noexcept
{
	m_maxJobs = std::max(inValue, 1U);
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::dumpAssembly() const noexcept
{
	return m_dumpAssembly;
}

void CommandLineInputs::setDumpAssembly(const bool inValue) noexcept
{
	m_dumpAssembly = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::showCommands() const noexcept
{
	return m_showCommands;
}

void CommandLineInputs::setShowCommands(const bool inValue) noexcept
{
	m_showCommands = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::benchmark() const noexcept
{
	return m_benchmark;
}

void CommandLineInputs::setBenchmark(const bool inValue) noexcept
{
	m_benchmark = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::launchProfiler() const noexcept
{
	return m_launchProfiler;
}
void CommandLineInputs::setLaunchProfiler(const bool inValue) noexcept
{
	m_launchProfiler = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::keepGoing() const noexcept
{
	return m_keepGoing;
}
void CommandLineInputs::setKeepGoing(const bool inValue) noexcept
{
	m_keepGoing = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::generateCompileCommands() const noexcept
{
	return m_generateCompileCommands;
}

void CommandLineInputs::setGenerateCompileCommands(const bool inValue) noexcept
{
	m_generateCompileCommands = inValue;
}

/*****************************************************************************/
const std::optional<bool>& CommandLineInputs::onlyRequired() const noexcept
{
	return m_onlyRequired;
}
void CommandLineInputs::setOnlyRequired(const bool inValue) noexcept
{
	m_onlyRequired = inValue;
}

/*****************************************************************************/
bool CommandLineInputs::saveUserToolchainGlobally() const noexcept
{
	return m_saveUserToolchainGlobally;
}
void CommandLineInputs::setSaveUserToolchainGlobally(const bool inValue) noexcept
{
	m_saveUserToolchainGlobally = inValue;
}

/*****************************************************************************/
StringList CommandLineInputs::getToolchainPresets() const
{
	StringList ret;

#if defined(CHALET_WIN32)
	auto visualStudioPresets = getVisualStudioPresets();
	for (auto it = visualStudioPresets.rbegin(); it != visualStudioPresets.rend(); ++it)
	{
		auto& [name, type] = *it;
		if (type == VisualStudioVersion::VisualStudio2015)
			break;

		ret.emplace_back(name);
	}
	auto visualStudioLLVMPresets = getVisualStudioLLVMPresets();
	for (auto& [name, _] : visualStudioLLVMPresets)
	{
		ret.emplace_back(name);
	}

	ret.emplace_back(kToolchainPresetLLVM);
	ret.emplace_back(kToolchainPresetGCC);
#elif defined(CHALET_MACOS)
	ret.emplace_back(kToolchainPresetAppleLLVM);
	ret.emplace_back(kToolchainPresetLLVM);
	ret.emplace_back(kToolchainPresetGCC);
#else
	ret.emplace_back(kToolchainPresetGCC);
	ret.emplace_back(kToolchainPresetLLVM);
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	#if defined(CHALET_WIN32)
	auto intelClangPresets = getIntelClangVSPresets();
	for (auto it = intelClangPresets.rbegin(); it != intelClangPresets.rend(); ++it)
	{
		auto& [name, type] = *it;
		ret.emplace_back(name);
	}
	#else
	ret.emplace_back(kToolchainPresetICX);
	#endif
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	auto intelClassicPresets = getIntelClassicVSPresets();
	for (auto it = intelClassicPresets.rbegin(); it != intelClassicPresets.rend(); ++it)
	{
		auto& [name, type] = *it;
		ret.emplace_back(name);
	}
	#else
	ret.emplace_back(kToolchainPresetICC);
	#endif
#endif

	return ret;
}

/*****************************************************************************/
StringList CommandLineInputs::getExportKindPresets() const
{
	StringList ret;

	auto exportTemplates = getExportKinds();
	for (auto& [name, _] : exportTemplates)
	{
		ret.emplace_back(name);
	}

	return ret;
}

/*****************************************************************************/
StringList CommandLineInputs::getProjectInitializationPresets() const
{
	StringList ret;

	auto initTemplates = getInitTemplates();
	for (auto& [name, _] : initTemplates)
	{
		ret.emplace_back(name);
	}

	return ret;
}

/*****************************************************************************/
StringList CommandLineInputs::getCliQueryOptions() const
{
	StringList ret;

	auto queryOptions = getQueryOptions();
	for (auto& [name, _] : queryOptions)
	{
		ret.emplace_back(name);
	}

	return ret;
}

/*****************************************************************************/
ToolchainPreference CommandLineInputs::getToolchainPreferenceFromString(const std::string& inValue) const
{
	ToolchainPreference ret;
	ret.buildPathStyle = BuildPathStyle::TargetTriple;
	ret.strategy = StrategyType::Ninja;

	m_isToolchainPreset = false;

	bool hasGccPrefix = String::startsWith("gcc-", inValue);
	bool hasGccSuffix = String::endsWith("-gcc", inValue);
	bool hasGccPrefixAndSuffix = String::contains("-gcc-", inValue);
	bool isGcc = String::equals(kToolchainPresetGCC, inValue);

	bool hasLlvmPrefix = String::startsWith("llvm-", inValue);

#if defined(CHALET_WIN32)
	m_visualStudioVersion = VisualStudioVersion::None;

	auto visualStudioPresets = getVisualStudioPresets();
	auto visualStudioLLVMPresets = getVisualStudioLLVMPresets();
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	auto intelClassicPresets = getIntelClassicVSPresets();
	#endif
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	auto intelClangPresets = getIntelClangVSPresets();
	#endif

	if (visualStudioPresets.find(inValue) != visualStudioPresets.end())
	{
		m_isToolchainPreset = true;
		m_isToolchainMultiArchPreset = true;
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);

		m_toolchainPreferenceName = inValue;

		ret.type = ToolchainType::VisualStudio;
		ret.cpp = "cl";
		ret.cc = "cl";
		ret.rc = "rc";
		ret.linker = "link";
		ret.archiver = "lib";
		ret.profiler = "vsinstr";
		ret.disassembler = "dumpbin";
	}
	else
#endif
#if defined(CHALET_MACOS)
		bool isAppleClang = String::equals(kToolchainPresetAppleLLVM, inValue);
	if (isAppleClang || String::equals(kToolchainPresetLLVM, inValue) || hasLlvmPrefix)
#else
	if (String::equals(kToolchainPresetLLVM, inValue) || hasLlvmPrefix)
#endif
	{
		m_isToolchainPreset = true;

#if defined(CHALET_WIN32)
		bool isVisualStudioLLVM = visualStudioLLVMPresets.find(inValue) != visualStudioLLVMPresets.end();
#endif

		std::string suffix;
		if (hasLlvmPrefix)
		{
#if defined(CHALET_WIN32)
			if (isVisualStudioLLVM)
			{
				m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);
			}
			else
#endif
			{
				suffix = inValue.substr(inValue.find_first_of('-'));
			}
		}

		m_toolchainPreferenceName = inValue;

		ret.disassembler = fmt::format("objdump{}", suffix);

#if defined(CHALET_MACOS)
		ret.type = isAppleClang ? ToolchainType::AppleLLVM : ToolchainType::LLVM;
#elif defined(CHALET_WIN32)
		ret.type = isVisualStudioLLVM ? ToolchainType::VisualStudioLLVM : ToolchainType::LLVM;
#else
		ret.type = ToolchainType::LLVM;
#endif
		ret.cpp = fmt::format("clang++{}", suffix);
		ret.cc = fmt::format("clang{}", suffix);
#if defined(CHALET_LINUX)
		ret.rc = fmt::format("llvm-windres{}", suffix);
#else
		ret.rc = fmt::format("llvm-rc{}", suffix);
#endif
		ret.linker = "lld";
		ret.archiver = "ar";
#if defined(CHALET_WIN32)
		ret.profiler = isVisualStudioLLVM ? "vsinstr" : "gprof";
#else
		ret.profiler = "gprof";
#endif
#if defined(CHALET_WIN32)
		ret.disassembler = "dumpbin";
#elif defined(CHALET_MACOS)
		ret.disassembler = "otool";
#else
		ret.disassembler = "objdump";
#endif
	}
	else if (isGcc || hasGccPrefix || hasGccSuffix || hasGccPrefixAndSuffix)
	{
		m_isToolchainPreset = true;
		if (isGcc)
			m_isToolchainMultiArchPreset = true;

		m_toolchainPreferenceName = inValue;

#if defined(CHALET_WIN32)
		ret.type = ToolchainType::MingwGNU;
#endif

		if (hasGccPrefixAndSuffix)
		{
			ret.cpp = inValue;
			ret.cc = inValue;
			ret.rc = inValue;
			ret.archiver = inValue;
			ret.linker = inValue;
			ret.disassembler = inValue;
			ret.profiler = inValue;
			String::replaceAll(ret.cpp, "-gcc-", "-g++-");
			String::replaceAll(ret.rc, "-gcc-", "-windres-");
			String::replaceAll(ret.archiver, "-gcc-", "-gcc-ar-");
			String::replaceAll(ret.linker, "-gcc-", "-ld-");
			String::replaceAll(ret.disassembler, "-gcc-", "-objdump-");
			String::replaceAll(ret.profiler, "-gcc-", "-gprof-");
		}
		else
		{
			std::string suffix;
			if (hasGccPrefix)
				suffix = inValue.substr(inValue.find_first_of('-'));

			std::string prefix;
			if (hasGccSuffix)
				prefix = inValue.substr(0, inValue.find_last_of('-') + 1);

			if (isGcc)
			{
				if (!m_targetArchitecture.empty() && m_targetArchitecture != m_hostArchitecture && ret.type != ToolchainType::MingwGNU)
				{
					m_targetArchitecture = getValidGccArchTripleFromArch(m_targetArchitecture);
					prefix = m_targetArchitecture + '-';
				}
			}

			ret.cpp = fmt::format("{}g++{}", prefix, suffix);
			ret.cc = fmt::format("{}gcc{}", prefix, suffix);
			ret.rc = fmt::format("{}windres{}", prefix, suffix);
			ret.archiver = fmt::format("{}gcc-ar{}", prefix, suffix); // gcc- will get stripped out later when it's searched
			ret.linker = fmt::format("{}ld{}", prefix, suffix);
			ret.disassembler = fmt::format("{}objdump{}", prefix, suffix);
			ret.profiler = fmt::format("{}gprof{}", prefix, suffix);

			if (!m_isToolchainMultiArchPreset)
			{
				m_toolchainPreferenceName = ret.cc;
			}
		}

#if !defined(CHALET_WIN32)
		if (String::contains("mingw", m_toolchainPreferenceName))
			ret.type = ToolchainType::MingwGNU;
		else
			ret.type = ToolchainType::GNU;
#endif
	}
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	#if defined(CHALET_WIN32)
	else if (intelClangPresets.find(inValue) != intelClangPresets.end())
	#else
	else if (String::equals(kToolchainPresetICX, inValue))
	#endif
	{
		m_isToolchainPreset = true;
		m_isToolchainMultiArchPreset = true;
		m_toolchainPreferenceName = inValue;
	#if defined(CHALET_WIN32)
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);
	#endif

		ret.type = ToolchainType::IntelLLVM;
		ret.cpp = "clang++";
		ret.cc = "clang";
		ret.rc = "rc";
		ret.linker = "lld";
		ret.archiver = "llvm-ar";
		ret.profiler = "";
		ret.disassembler = "dumpbin";
	}
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	else if (intelClassicPresets.find(inValue) != intelClassicPresets.end())
	#else
	else if (String::equals(kToolchainPresetICC, inValue))
	#endif
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;
	#if defined(CHALET_WIN32)
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);
	#endif

		ret.type = ToolchainType::IntelClassic;
		ret.rc = "rc";
	#if defined(CHALET_WIN32)
		ret.cpp = "icl";
		ret.cc = "icl";
		ret.linker = "xilink";
		ret.archiver = "xilib";
		ret.profiler = "";
		ret.disassembler = "dumpbin";
	#else
		ret.cpp = "icpc";
		ret.cc = "icc";
		ret.linker = "xild";
		ret.archiver = "xiar";
		ret.profiler = "gprof";
		ret.disassembler = "objdump";
	#endif
	}
#endif
	else
	{
		ret.type = ToolchainType::Unknown;
	}

	return ret;
}

/*****************************************************************************/
ExportKind CommandLineInputs::getExportKindFromString(const std::string& inValue) const
{
	auto exportKinds = getExportKinds();
	if (exportKinds.find(inValue) != exportKinds.end())
	{
		return exportKinds.at(inValue);
	}

	return ExportKind::None;
}

/*****************************************************************************/
QueryOption CommandLineInputs::getQueryOptionFromString(const std::string& inValue) const
{
	auto queryOptions = getQueryOptions();
	if (queryOptions.find(inValue) != queryOptions.end())
	{
		return queryOptions.at(inValue);
	}

	return QueryOption::None;
}

/*****************************************************************************/
VisualStudioVersion CommandLineInputs::getVisualStudioVersionFromPresetString(const std::string& inValue) const
{
#if defined(CHALET_WIN32)
	auto presets = getVisualStudioPresets();
	if (presets.find(inValue) != presets.end())
		return presets.at(inValue);

	presets = getVisualStudioLLVMPresets();
	if (presets.find(inValue) != presets.end())
		return presets.at(inValue);

	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	presets = getIntelClassicVSPresets();
	if (presets.find(inValue) != presets.end())
		return presets.at(inValue);
	#endif
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	presets = getIntelClangVSPresets();
	if (presets.find(inValue) != presets.end())
		return presets.at(inValue);
	#endif
#else
	UNUSED(inValue);
#endif

	return VisualStudioVersion::Stable;
}

/*****************************************************************************/
InitTemplateType CommandLineInputs::getInitTemplateFromString(const std::string& inValue) const
{
	auto initTemplates = getInitTemplates();
	if (initTemplates.find(inValue) != initTemplates.end())
	{
		return initTemplates.at(inValue);
	}

	return InitTemplateType::None;
}

/*****************************************************************************/
std::string CommandLineInputs::getValidGccArchTripleFromArch(const std::string& inArch) const
{
#if defined(CHALET_LINUX)
	auto firstDash = inArch.find_first_of('-');
	if (firstDash == std::string::npos)
	{
		auto gcc = Commands::which("gcc");
		if (!gcc.empty())
		{
			auto cachedArch = Commands::subprocessOutput({ gcc, "-dumpmachine" });
			firstDash = cachedArch.find_first_of('-');

			bool valid = !cachedArch.empty() && firstDash != std::string::npos;
			if (valid)
			{
				auto suffix = cachedArch.substr(firstDash);

				auto arch = Arch::from(inArch);
				std::string targetArch = arch.str;
				if (arch.val == Arch::Cpu::ARMHF)
				{
					suffix += "eabihf";
					targetArch = "arm";
				}
				else if (arch.val == Arch::Cpu::ARM)
				{
					suffix += "eabi";
					targetArch = "arm";
				}
				else if (arch.val == Arch::Cpu::ARM64)
				{
					targetArch = "aarch64";
				}

				cachedArch = fmt::format("{}{}", targetArch, suffix);

				auto searchPathA = fmt::format("/usr/lib/gcc/{}", cachedArch);
				auto searchPathB = fmt::format("/usr/lib/gcc-cross/{}", cachedArch);

				bool found = Commands::pathExists(searchPathA) || Commands::pathExists(searchPathB);
				if (!found && String::startsWith("-pc-linux-gnu", suffix))
				{
					suffix = suffix.substr(3);
					cachedArch = fmt::format("{}{}", targetArch, suffix);

					searchPathA = fmt::format("/usr/lib/gcc/{}", cachedArch);
					searchPathB = fmt::format("/usr/lib/gcc-cross/{}", cachedArch);

					found = Commands::pathExists(searchPathA) || Commands::pathExists(searchPathB);
				}

				if (found)
				{
					return cachedArch;
				}
			}
		}
	}
#endif

	m_isToolchainMultiArchPreset = false;

	return inArch;
}
}
