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
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
static struct
{
	const Dictionary<IdeType> ideTypes{
		{ "vs2019", IdeType::VisualStudio2019 },
		{ "vscode", IdeType::VisualStudioCode },
		{ "xcode", IdeType::XCode },
		// { "codeblocks", IdeType::CodeBlocks },
	};

	const Dictionary<QueryOption> queryOptions{
		{ "all-toolchains", QueryOption::AllToolchains },
		{ "architecture", QueryOption::Architecture },
		{ "architectures", QueryOption::Architectures },
		{ "commands", QueryOption::Commands },
		{ "configuration", QueryOption::Configuration },
		{ "configurations", QueryOption::Configurations },
		{ "list-names", QueryOption::QueryNames },
		{ "run-target", QueryOption::RunTarget },
		{ "theme-names", QueryOption::ThemeNames },
		{ "toolchain", QueryOption::Toolchain },
		{ "toolchain-presets", QueryOption::ToolchainPresets },
		{ "user-toolchains", QueryOption::UserToolchains },
	};

#if defined(CHALET_WIN32)
	const OrderedDictionary<VisualStudioVersion> visualStudioPresets{
		{ "vs-2010", VisualStudioVersion::VisualStudio2010 },
		{ "vs-2012", VisualStudioVersion::VisualStudio2012 },
		{ "vs-2013", VisualStudioVersion::VisualStudio2013 },
		{ "vs-2015", VisualStudioVersion::VisualStudio2015 },
		{ "vs-2017", VisualStudioVersion::VisualStudio2017 },
		{ "vs-2019", VisualStudioVersion::VisualStudio2019 },
		{ "vs-2022", VisualStudioVersion::VisualStudio2022 },
		{ "vs-preview", VisualStudioVersion::Preview },
		{ "vs-stable", VisualStudioVersion::Stable },
	};
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	const OrderedDictionary<VisualStudioVersion> intelICCVisualStudioPresets{
		{ "intel-classic-vs-2017", VisualStudioVersion::VisualStudio2017 },
		{ "intel-classic-vs-2019", VisualStudioVersion::VisualStudio2019 },
		// { "intel-classic-vs-2022", VisualStudioVersion::VisualStudio2022 },
	};
	#endif
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	const OrderedDictionary<VisualStudioVersion> intelICXVisualStudioPresets{
		{ "intel-llvm-vs-2017", VisualStudioVersion::VisualStudio2017 },
		{ "intel-llvm-vs-2019", VisualStudioVersion::VisualStudio2019 },
		// { "intel-llvm-vs-2022", VisualStudioVersion::VisualStudio2022 },
	};
	#endif
#endif
} state;
}

/*****************************************************************************/
CommandLineInputs::CommandLineInputs() :
	kDefaultInputFile("chalet.json"),
	kDefaultSettingsFile(".chaletrc"),
	kDefaultEnvFile(".env"),
	kDefaultOutputDirectory("build"),
	kDefaultExternalDirectory("chalet_external"),
	kDefaultDistributionDirectory("dist"),
	kGlobalSettingsFile(".chaletconfig"),
	kArchPresetAuto("auto"),
	kToolchainPresetGCC("gcc"),
	kToolchainPresetLLVM("llvm"),
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC && !defined(CHALET_WIN32)
	kToolchainPresetICC("intel-classic"),
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	kToolchainPresetICX("intel-llvm"),
#endif
#if defined(CHALET_WIN32)
	kToolchainPresetVisualStudioStable("vs-stable"),
#elif defined(CHALET_MACOS)
	kToolchainPresetAppleLLVM("apple-llvm"),
#endif
	m_settingsFile(kDefaultSettingsFile),
	m_hostArchitecture(Arch::getHostCpuArchitecture())
{
	// LOG("Processor count: ", m_processorCount);
}

/*****************************************************************************/
void CommandLineInputs::detectToolchainPreference() const
{
	if (!m_toolchainPreferenceName.empty())
		return;

	const auto& defaultPreset = defaultToolchainPreset();
	m_toolchainPreference = getToolchainPreferenceFromString(defaultPreset);
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
Route CommandLineInputs::route() const noexcept
{
	return m_route;
}

void CommandLineInputs::setRoute(const Route inValue) noexcept
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

	m_buildConfiguration = inValue;
}

/*****************************************************************************/
const std::string& CommandLineInputs::runTarget() const noexcept
{
	return m_runTarget;
}

void CommandLineInputs::setRunTarget(std::string&& inValue) noexcept
{
	if (inValue.empty())
		return;

	m_runTarget = std::move(inValue);
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
VisualStudioVersion CommandLineInputs::visualStudioVersion() const noexcept
{
	return m_visualStudioVersion;
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

	if (String::equals(kArchPresetAuto, inValue))
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
		cwd[0] = static_cast<char>(::tolower(cwd.front()));
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
const std::optional<bool>& CommandLineInputs::generateCompileCommands() const noexcept
{
	return m_generateCompileCommands;
}

void CommandLineInputs::setGenerateCompileCommands(const bool inValue) noexcept
{
	m_generateCompileCommands = inValue;
}

/*****************************************************************************/
StringList CommandLineInputs::getToolchainPresets() const noexcept
{
	StringList ret;

#if defined(CHALET_WIN32)
	for (auto it = state.visualStudioPresets.rbegin(); it != state.visualStudioPresets.rend(); ++it)
	{
		auto& [name, type] = *it;
		if (type == VisualStudioVersion::VisualStudio2015)
			break;

		ret.emplace_back(name);
	}
#elif defined(CHALET_MACOS)
	ret.emplace_back(kToolchainPresetAppleLLVM);
#endif
	ret.emplace_back(kToolchainPresetLLVM);
	ret.emplace_back(kToolchainPresetGCC);
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	ret.emplace_back(kToolchainPresetICX);
	#if defined(CHALET_WIN32)
	for (auto it = state.intelICXVisualStudioPresets.rbegin(); it != state.intelICXVisualStudioPresets.rend(); ++it)
	{
		auto& [name, type] = *it;
		ret.emplace_back(name);
	}
	#endif
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	for (auto it = state.intelICCVisualStudioPresets.rbegin(); it != state.intelICCVisualStudioPresets.rend(); ++it)
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
StringList CommandLineInputs::getCliQueryOptions() const noexcept
{
	StringList ret;

	for (auto& [name, _] : state.queryOptions)
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

	m_isToolchainPreset = false;

	bool hasGccPrefix = String::startsWith("gcc-", inValue);
	bool hasGccSuffix = String::endsWith("-gcc", inValue);
	bool hasGccPrefixAndSuffix = String::contains("-gcc-", inValue);

#if defined(CHALET_WIN32)
	m_visualStudioVersion = VisualStudioVersion::None;

	if (state.visualStudioPresets.find(inValue) != state.visualStudioPresets.end())
	{
		m_isToolchainPreset = true;
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);

		m_toolchainPreferenceName = inValue;

		ret.type = ToolchainType::VisualStudio;
		ret.strategy = StrategyType::Ninja;
		ret.buildPathStyle = BuildPathStyle::ToolchainName;
		ret.cpp = "cl";
		ret.cc = "cl";
		ret.rc = "rc";
		ret.linker = "link";
		ret.archiver = "lib";
		ret.profiler = ""; // TODO
		ret.disassembler = "dumpbin";
	}
	else
#endif
#if defined(CHALET_MACOS)
		bool isAppleClang = String::equals(kToolchainPresetAppleLLVM, inValue);
	if (isAppleClang || String::equals(kToolchainPresetLLVM, inValue))
#else
	if (String::equals(kToolchainPresetLLVM, inValue))
#endif
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;

#if defined(CHALET_MACOS)
		ret.type = isAppleClang ? ToolchainType::AppleLLVM : ToolchainType::LLVM;
#else
		ret.type = ToolchainType::LLVM;
#endif
		ret.strategy = StrategyType::Ninja;
		ret.cpp = "clang++";
		ret.cc = "clang";
		ret.rc = "llvm-rc";
		ret.linker = "lld";
		ret.archiver = "ar";
		ret.profiler = ""; // TODO
#if defined(CHALET_MACOS)
		ret.disassembler = "otool";
#else
		ret.disassembler = "objdump";
#endif
	}
	else if (String::equals(kToolchainPresetGCC, inValue) || hasGccPrefix || hasGccSuffix || hasGccPrefixAndSuffix)
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;

		std::string suffix;
		if (hasGccPrefix)
			suffix = inValue.substr(inValue.find_first_of('-'));

		std::string prefix;
		if (hasGccSuffix)
			prefix = inValue.substr(0, inValue.find_last_of('-') + 1);

		if (String::contains("mingw", inValue))
			ret.type = ToolchainType::MingwGNU;
		else
			ret.type = ToolchainType::GNU;

		ret.strategy = StrategyType::Ninja;
		if (hasGccPrefixAndSuffix)
		{
			ret.cpp = inValue;
			ret.cc = inValue;
			ret.archiver = inValue;
			ret.linker = inValue;
			ret.disassembler = inValue;
			ret.profiler = inValue;
			String::replaceAll(ret.cpp, "-gcc-", "-g++-");
			String::replaceAll(ret.archiver, "-gcc-", "-gcc-ar-");
			String::replaceAll(ret.linker, "-gcc-", "-ld-");
			String::replaceAll(ret.disassembler, "-gcc-", "-objdump-");
			String::replaceAll(ret.profiler, "-gcc-", "-gprof-");
		}
		else
		{
			ret.cpp = fmt::format("{}g++{}", prefix, suffix);
			ret.cc = fmt::format("{}gcc{}", prefix, suffix);
			ret.archiver = fmt::format("{}gcc-ar{}", prefix, suffix); // gcc- will get stripped out later when it's searched
			ret.linker = fmt::format("{}ld{}", prefix, suffix);
			ret.disassembler = fmt::format("{}objdump{}", prefix, suffix);
			ret.profiler = fmt::format("{}gprof{}", prefix, suffix);
		}
		ret.rc = "windres";
	}
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	#if defined(CHALET_WIN32)
	else if (String::equals(kToolchainPresetICX, inValue) || state.intelICXVisualStudioPresets.find(inValue) != state.intelICXVisualStudioPresets.end())
	#else
	else if (String::equals(kToolchainPresetICX, inValue))
	#endif
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;
	#if defined(CHALET_WIN32)
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);
	#endif

		ret.type = ToolchainType::IntelLLVM;
		ret.strategy = StrategyType::Ninja;
		ret.rc = "rc";
		ret.cpp = "clang++";
		ret.cc = "clang";
		ret.linker = "lld";
		ret.archiver = "llvm-ar";
		ret.profiler = "gprof";
		ret.disassembler = "dumpbin";
	}
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	else if (state.intelICCVisualStudioPresets.find(inValue) != state.intelICCVisualStudioPresets.end())
	#else
	else if (String::equals(kToolchainPresetICC, inValue))
	#endif
	{
		m_isToolchainPreset = true;
		m_toolchainPreferenceName = inValue;
	#if defined(CHALET_WIN32)
		m_visualStudioVersion = getVisualStudioVersionFromPresetString(inValue);
	#endif

		ret.strategy = StrategyType::Ninja;
		ret.type = ToolchainType::IntelClassic;
		ret.rc = "rc";
	#if defined(CHALET_WIN32)
		ret.cpp = "icl";
		ret.cc = "icl";
		ret.linker = "xilink";
		ret.archiver = "xilib";
		ret.profiler = ""; // TODO
		ret.disassembler = "dumpbin";
	#else
		ret.cpp = "icpc";
		ret.cc = "icc";
		ret.linker = "xild";
		ret.archiver = "xiar";
		ret.profiler = ""; // TODO
		ret.disassembler = "objdump";
	#endif
	}
#endif
	else
	{
		ret.type = ToolchainType::Unknown;
	}

	if (ret.type != ToolchainType::VisualStudio)
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
	if (state.ideTypes.find(inValue) != state.ideTypes.end())
	{
		return state.ideTypes.at(inValue);
	}
	else if (!inValue.empty())
	{
		return IdeType::Unknown;
	}

	return IdeType::None;
}

/*****************************************************************************/
QueryOption CommandLineInputs::getQueryOptionFromString(const std::string& inValue) const
{
	if (state.queryOptions.find(inValue) != state.queryOptions.end())
	{
		return state.queryOptions.at(inValue);
	}

	return QueryOption::None;
}

/*****************************************************************************/
VisualStudioVersion CommandLineInputs::getVisualStudioVersionFromPresetString(const std::string& inValue) const
{
#if defined(CHALET_WIN32)
	if (state.visualStudioPresets.find(inValue) != state.visualStudioPresets.end())
	{
		return state.visualStudioPresets.at(inValue);
	}

	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	if (state.intelICCVisualStudioPresets.find(inValue) != state.intelICCVisualStudioPresets.end())
	{
		return state.intelICCVisualStudioPresets.at(inValue);
	}
	#endif
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	if (state.intelICXVisualStudioPresets.find(inValue) != state.intelICXVisualStudioPresets.end())
	{
		return state.intelICXVisualStudioPresets.at(inValue);
	}
	#endif
#else
	UNUSED(inValue);
#endif

	return VisualStudioVersion::Stable;
}
}
