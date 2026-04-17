/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/GlobalSettingsJsonFile.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
bool GlobalSettingsJsonFile::read(WorkspaceCache& inCache, IntermediateSettingsState& inFallback, bool& outShouldPerformUpdateCheck)
{
	GlobalSettingsJsonFile globalSettingsJsonFile(inCache, inFallback, outShouldPerformUpdateCheck);
	return globalSettingsJsonFile.deserialize();
}

/*****************************************************************************/
GlobalSettingsJsonFile::GlobalSettingsJsonFile(WorkspaceCache& inCache, IntermediateSettingsState& inFallback, bool& outShouldPerformUpdateCheck) :
	m_jsonFile(inCache.getSettings(SettingsType::Global)),
	m_fallback(inFallback),
	m_shouldPerformUpdateCheck(outShouldPerformUpdateCheck)
{}

/*****************************************************************************/
bool GlobalSettingsJsonFile::deserialize()
{
	auto& jRoot = m_jsonFile.root;
	if (!jRoot.is_object())
		jRoot = Json::object();

	bool dirty = false;
	dirty |= json::assignNodeFromDataType(jRoot, Keys::Options, JsonDataType::object);
	dirty |= json::assignNodeFromDataType(jRoot, Keys::Toolchains, JsonDataType::object);
	dirty |= json::assignNodeFromDataType(jRoot, Keys::Tools, JsonDataType::object);

#if defined(CHALET_MACOS)
	dirty |= json::assignNodeFromDataType(jRoot, Keys::AppleSdks, JsonDataType::object);
#endif

	Json& jOptions = m_jsonFile.root[Keys::Options];

	{
		// pre 6.0.0
		const std::string kRunTarget{ "runTarget" };
		if (jOptions.contains(kRunTarget))
		{
			jOptions.erase(kRunTarget);
			dirty = true;
		}
	}

	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsDumpAssembly, m_fallback.dumpAssembly);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsShowCommands, m_fallback.showCommands);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsBenchmark, m_fallback.benchmark);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsLaunchProfiler, m_fallback.launchProfiler);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsKeepGoing, m_fallback.keepGoing);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsCompilerCache, m_fallback.compilerCache);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsGenerateCompileCommands, m_fallback.generateCompileCommands);
	dirty |= json::assignNodeIfEmpty<bool>(jOptions, Keys::OptionsOnlyRequired, m_fallback.onlyRequired);
	dirty |= json::assignNodeIfEmpty<u32>(jOptions, Keys::OptionsMaxJobs, m_fallback.maxJobs);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsBuildConfiguration, m_fallback.buildConfiguration);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsToolchain, m_fallback.toolchainPreference);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsArchitecture, m_fallback.architecturePreference);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsInputFile, m_fallback.inputFile);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsEnvFile, m_fallback.envFile);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsOutputDirectory, m_fallback.outputDirectory);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsExternalDirectory, m_fallback.externalDirectory);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsDistributionDirectory, m_fallback.distributionDirectory);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsOsTargetName, m_fallback.osTargetName);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsOsTargetVersion, m_fallback.osTargetVersion);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsSigningIdentity, m_fallback.signingIdentity);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsProfilerConfig, m_fallback.profilerConfig);
	dirty |= json::assignNodeIfEmpty<std::string>(jOptions, Keys::OptionsLastTarget, m_fallback.lastTarget);

	// Root directory should never be set globally
	jOptions[Keys::OptionsRootDirectory] = std::string();
	dirty |= json::assignNodeFromDataType(jOptions, Keys::OptionsRunArguments, JsonDataType::object);

	reassignIntermediateStateFromSettings(m_fallback, jRoot);

	dirty |= assignDefaultTheme(jRoot);
	dirty |= assignLastUpdate(jRoot);

	if (dirty)
		m_jsonFile.setDirty(true);

	return true;
}

/*****************************************************************************/
void GlobalSettingsJsonFile::reassignIntermediateStateFromSettings(IntermediateSettingsState& outState, const Json& inNode) const
{
	const Json& jOptions = inNode[Keys::Options];
	for (const auto& [key, value] : jOptions.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::OptionsBuildConfiguration, key))
				outState.buildConfiguration = value.get<std::string>();
			else if (String::equals(Keys::OptionsToolchain, key))
				outState.toolchainPreference = value.get<std::string>();
			else if (String::equals(Keys::OptionsArchitecture, key))
				outState.architecturePreference = value.get<std::string>();
			else if (String::equals(Keys::OptionsSigningIdentity, key))
				outState.signingIdentity = value.get<std::string>();
			else if (String::equals(Keys::OptionsProfilerConfig, key))
				outState.profilerConfig = value.get<std::string>();
			else if (String::equals(Keys::OptionsOsTargetName, key))
				outState.osTargetName = value.get<std::string>();
			else if (String::equals(Keys::OptionsOsTargetVersion, key))
				outState.osTargetVersion = value.get<std::string>();
			else if (String::equals(Keys::OptionsLastTarget, key))
				outState.lastTarget = value.get<std::string>();
			else if (String::equals(Keys::OptionsInputFile, key))
				outState.inputFile = value.get<std::string>();
			else if (String::equals(Keys::OptionsEnvFile, key))
				outState.envFile = value.get<std::string>();
			// else if (String::equals(Keys::OptionsRootDirectory, key))
			// 	outState.rootDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsOutputDirectory, key))
				outState.outputDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsExternalDirectory, key))
				outState.externalDirectory = value.get<std::string>();
			else if (String::equals(Keys::OptionsDistributionDirectory, key))
				outState.distributionDirectory = value.get<std::string>();
		}
		else if (value.is_boolean())
		{
			if (String::equals(Keys::OptionsShowCommands, key))
				outState.showCommands = value.get<bool>();
			else if (String::equals(Keys::OptionsDumpAssembly, key))
				outState.dumpAssembly = value.get<bool>();
			else if (String::equals(Keys::OptionsBenchmark, key))
				outState.benchmark = value.get<bool>();
			else if (String::equals(Keys::OptionsLaunchProfiler, key))
				outState.launchProfiler = value.get<bool>();
			else if (String::equals(Keys::OptionsKeepGoing, key))
				outState.keepGoing = value.get<bool>();
			else if (String::equals(Keys::OptionsCompilerCache, key))
				outState.compilerCache = value.get<bool>();
			else if (String::equals(Keys::OptionsGenerateCompileCommands, key))
				outState.generateCompileCommands = value.get<bool>();
			else if (String::equals(Keys::OptionsOnlyRequired, key))
				outState.onlyRequired = value.get<bool>();
		}
		else if (value.is_number())
		{
			if (String::equals(Keys::OptionsMaxJobs, key))
				outState.maxJobs = static_cast<u32>(value.get<i32>());
		}
	}

	outState.toolchains = inNode[Keys::Toolchains];
	outState.tools = inNode[Keys::Tools];
#if defined(CHALET_MACOS)
	outState.appleSdks = inNode[Keys::AppleSdks];
#endif
}

/*****************************************************************************/
bool GlobalSettingsJsonFile::assignDefaultTheme(Json& outNode)
{
	if (outNode.contains(Keys::Theme))
	{
		Json& jTheme = outNode[Keys::Theme];
		if (jTheme.is_string())
		{
			// If the preset is valid, we don't assign it
			auto preset = jTheme.get<std::string>();
			if (ColorTheme::isValidPreset(preset))
				return false;
		}
		else if (jTheme.is_object())
		{
			// If it's been set to an object, we validate each color

			bool dirty = false;

			const auto& theme = Output::theme();
			auto colorKeys = ColorTheme::getKeys();
			for (const auto& key : colorKeys)
			{
				if (!json::isValid<std::string>(jTheme, key.c_str()))
				{
					jTheme[key] = theme.getAsString(key);
					dirty = true;
				}
			}

			return dirty;
		}
	}

	outNode[Keys::Theme] = ColorTheme::getDefaultPresetName();
	return true;
}

/*****************************************************************************/
bool GlobalSettingsJsonFile::assignLastUpdate(Json& outNode)
{
	time_t lastUpdateCheck = 0;
	time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	if (outNode.contains(Keys::LastUpdateCheck))
	{
		if (outNode[Keys::LastUpdateCheck].is_number_unsigned())
		{
			lastUpdateCheck = m_jsonFile.root[Keys::LastUpdateCheck].get<time_t>();
			m_shouldPerformUpdateCheck = shouldPerformUpdateCheckBasedOnLastUpdate(lastUpdateCheck, currentTime);
		}
	}

	outNode[Keys::LastUpdateCheck] = m_shouldPerformUpdateCheck ? currentTime : lastUpdateCheck;

	return m_shouldPerformUpdateCheck;
}

/*****************************************************************************/
bool GlobalSettingsJsonFile::shouldPerformUpdateCheckBasedOnLastUpdate(const time_t inLastUpdate, const time_t inCurrent) const
{
	time_t difference = inCurrent - inLastUpdate;

	// LOG("lastUpdateCheck:", inLastUpdate);
	// LOG("currentTime:", inCurrent);
	// LOG("difference:", difference);

	time_t checkDuration = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::hours(24)).count();
	// time_t checkDuration = 5;
	// LOG("checkDuration:", checkDuration);

	return difference < 0 || difference >= checkDuration;
}

}
