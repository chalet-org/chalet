/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonFile.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp" // getAllowedSDKTargets
#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"

#include "Process/Process.hpp"
#include "State/CentralState.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
bool SettingsJsonFile::read(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback)
{
	auto& jsonFile = inCentralState.cache.getSettings(SettingsType::Local);
	SettingsJsonFile settingsJsonFile(inInputs, inCentralState, inFallback);
	return settingsJsonFile.readFrom(jsonFile);
}

/*****************************************************************************/
SettingsJsonFile::SettingsJsonFile(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback) :
	m_inputs(inInputs),
	m_centralState(inCentralState),
	m_fallback(inFallback)
{}

/*****************************************************************************/
bool SettingsJsonFile::readFrom(JsonFile& inJsonFile)
{
	auto& jRoot = inJsonFile.root;
	if (!jRoot.is_object())
		jRoot = Json::object();

	bool dirty = false;
	dirty |= json::assignObjectNodeIfInvalid(jRoot, Keys::Options);
	dirty |= json::assignObjectNodeIfInvalid(jRoot, Keys::Toolchains, m_fallback.toolchains);
	dirty |= json::assignObjectNodeIfInvalidAndIncludeMissingPairs(jRoot, Keys::Tools, m_fallback.tools);
#if defined(CHALET_MACOS)
	dirty |= json::assignObjectNodeIfInvalidAndIncludeMissingPairs(jRoot, Keys::AppleSdks, m_fallback.appleSdks);
#endif

	auto& jOptions = jRoot[Keys::Options];

	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsDumpAssembly, m_inputs.dumpAssembly(), m_fallback.dumpAssembly);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsShowCommands, m_inputs.showCommands(), m_fallback.showCommands);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsBenchmark, m_inputs.benchmark(), m_fallback.benchmark);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsLaunchProfiler, m_inputs.launchProfiler(), m_fallback.launchProfiler);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsKeepGoing, m_inputs.keepGoing(), m_fallback.keepGoing);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsCompilerCache, m_inputs.compilerCache(), m_fallback.compilerCache);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsGenerateCompileCommands, m_inputs.generateCompileCommands(), m_fallback.generateCompileCommands);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsOnlyRequired, m_inputs.onlyRequired(), m_fallback.onlyRequired);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsMaxJobs, m_inputs.maxJobs(), m_fallback.maxJobs);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsToolchain, m_inputs.toolchainPreferenceName(), m_fallback.toolchainPreference);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsBuildConfiguration, m_inputs.buildConfiguration(), m_fallback.buildConfiguration);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsArchitecture, m_inputs.architectureRaw(), m_fallback.architecturePreference);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsInputFile, m_inputs.inputFile(), m_fallback.inputFile);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsEnvFile, m_inputs.envFile(), m_fallback.envFile);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsRootDirectory, m_inputs.rootDirectory(), m_fallback.rootDirectory);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsOutputDirectory, m_inputs.outputDirectory(), m_fallback.outputDirectory);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsExternalDirectory, m_inputs.externalDirectory(), m_fallback.externalDirectory);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsDistributionDirectory, m_inputs.distributionDirectory(), m_fallback.distributionDirectory);

	// We always want to save these values
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsOsTargetName, m_inputs.osTargetName(), m_fallback.osTargetName);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsOsTargetVersion, m_inputs.osTargetVersion(), m_fallback.osTargetVersion);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsSigningIdentity, m_inputs.signingIdentity(), m_fallback.signingIdentity);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsProfilerConfig, m_inputs.profilerConfig(), m_fallback.profilerConfig);
	dirty |= json::assignNodeIfEmptyWithFallback(jOptions, Keys::OptionsLastTarget, m_inputs.lastTarget(), m_fallback.lastTarget);

	dirty |= json::assignObjectNodeIfInvalid(jOptions, Keys::OptionsRunArguments);

	//

	auto detectPathAndAssignNode = [](Json& inNode, const char* inKey) -> bool {
		if (!inNode.contains(inKey))
		{
			auto path = Files::which(inKey);
#if defined(CHALET_WIN32)
			String::replaceAll(path, "WINDOWS/SYSTEM32", "Windows/System32");
#endif
			bool res = !path.empty();
			inNode[inKey] = std::move(path);
			return res;
		}

		return false;
	};

	auto& jTools = jRoot[Keys::Tools];

	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsBash);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsCcache);
#if defined(CHALET_MACOS)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsCodesign);
#endif

#if defined(CHALET_WIN32)
	if (!jTools.contains(Keys::ToolsCommandPrompt))
	{
		auto path = Files::which("cmd");
		String::replaceAll(path, "WINDOWS/SYSTEM32", "Windows/System32");
		jTools[Keys::ToolsCommandPrompt] = std::move(path);
		dirty = true;
	}
#endif
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsCurl);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsGit);
#if defined(CHALET_MACOS)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsHdiutil);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsInstallNameTool);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsInstruments);
#endif
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsLdd);

#if !defined(CHALET_WIN32)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsShasum);
#endif
#if defined(CHALET_MACOS)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsOsascript);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsOtool);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsPlutil);
#endif

	if (!jTools.contains(Keys::ToolsPowershell))
	{
		auto powershell = Files::which("pwsh"); // Powershell OS 6+ (ex: C:/Program Files/Powershell/6)
#if defined(CHALET_WIN32)
		if (powershell.empty())
			powershell = Files::which(Keys::ToolsPowershell);
#endif
		jTools[Keys::ToolsPowershell] = std::move(powershell);
		dirty = true;
	}

#if defined(CHALET_MACOS)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsSample);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsSips);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsTar);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsTiffutil);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsXcodebuild);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsXcrun);
#else
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsTar);
#endif

#if !defined(CHALET_WIN32)
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsUnzip);
	dirty |= detectPathAndAssignNode(jTools, Keys::ToolsZip);
#endif

#if defined(CHALET_WIN32)
	dirty |= detectPathsToGitAndLLD(jTools);
#elif defined(CHALET_MACOS)
	dirty |= detectPathsToPlatformSDKs(jRoot);
#endif

	// Removed in version 6.0.0
	dirty |= json::removeNode(jOptions, "runTarget");

	// The only reason it would be in the local settings if a user added it
	dirty |= json::removeNode(jOptions, Keys::LastUpdateCheck);

	if (dirty)
		inJsonFile.setDirty(true);

	if (!validateFileContentsAgainstSchema(inJsonFile))
		return false;

	dirty = false;
	dirty |= readFromSettings(jRoot);
	dirty |= readFromTools(jRoot);
	readFromPlatformSDKs(jRoot);

	if (dirty)
		inJsonFile.setDirty(true);

	if (!validatePlatformSDKs(inJsonFile))
		return false;

	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::validateFileContentsAgainstSchema(const JsonFile& inJsonFile)
{
	Json schema = SettingsJsonSchema::get(m_inputs);
	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(schema, "schema/chalet-settings.schema.json");
	}

	if (!inJsonFile.validate(schema))
		return false;

	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::readFromSettings(Json& inNode)
{
	chalet_assert(json::isObject(inNode, Keys::Options), "required node is invalid");

	auto& jOptions = inNode[Keys::Options];

	StringList removeKeys;
	for (const auto& [key, value] : jOptions.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::OptionsBuildConfiguration, key))
			{
				if (m_inputs.buildConfiguration().empty())
					m_inputs.setBuildConfiguration(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsToolchain, key))
			{
				if (m_inputs.toolchainPreferenceName().empty())
					m_inputs.setToolchainPreference(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsArchitecture, key))
			{
				if (m_inputs.architectureRaw().empty())
					m_inputs.setArchitectureRaw(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsLastTarget, key))
			{
				if (m_inputs.lastTarget().empty())
					m_inputs.setLastTarget(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsSigningIdentity, key))
			{
				if (m_inputs.signingIdentity().empty())
					m_inputs.setSigningIdentity(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsProfilerConfig, key))
			{
				if (m_inputs.profilerConfig().empty())
					m_inputs.setProfilerConfig(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsOsTargetName, key))
			{
				if (m_inputs.osTargetName().empty())
					m_inputs.setOsTargetName(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsOsTargetVersion, key))
			{
				if (m_inputs.osTargetVersion().empty())
					m_inputs.setOsTargetVersion(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsInputFile, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.inputFile().empty() || !String::equals(StringList{ m_inputs.inputFile(), m_inputs.defaultInputFile() }, val))
					m_inputs.setInputFile(std::move(val));
			}
			else if (String::equals(Keys::OptionsEnvFile, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.envFile().empty() || !String::equals(StringList{ m_inputs.envFile(), m_inputs.defaultEnvFile() }, val))
					m_inputs.setEnvFile(std::move(val));
			}
			else if (String::equals(Keys::OptionsRootDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.rootDirectory().empty() || !String::equals(m_inputs.rootDirectory(), val))
					m_inputs.setRootDirectory(std::move(val));
			}
			else if (String::equals(Keys::OptionsOutputDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.outputDirectory().empty() || !String::equals(StringList{ m_inputs.outputDirectory(), m_inputs.defaultOutputDirectory() }, val))
					m_inputs.setOutputDirectory(std::move(val));
			}
			else if (String::equals(Keys::OptionsExternalDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.externalDirectory().empty() || !String::equals(StringList{ m_inputs.externalDirectory(), m_inputs.defaultExternalDirectory() }, val))
					m_inputs.setExternalDirectory(std::move(val));
			}
			else if (String::equals(Keys::OptionsDistributionDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.distributionDirectory().empty() || !String::equals(StringList{ m_inputs.distributionDirectory(), m_inputs.defaultDistributionDirectory() }, val))
					m_inputs.setDistributionDirectory(std::move(val));
			}
			else
				removeKeys.push_back(key);
		}
		else if (value.is_boolean())
		{
			if (String::equals(Keys::OptionsDumpAssembly, key))
			{
				if (!m_inputs.dumpAssembly().has_value())
					m_inputs.setDumpAssembly(value.get<bool>());
			}
			else if (String::equals(Keys::OptionsShowCommands, key))
			{
				bool val = value.get<bool>();

				if (m_inputs.showCommands().has_value())
					val = *m_inputs.showCommands();

				Output::setShowCommands(val);
			}
			else if (String::equals(Keys::OptionsBenchmark, key))
			{
				bool val = value.get<bool>();

				if (m_inputs.benchmark().has_value())
					val = *m_inputs.benchmark();

				Output::setShowBenchmarks(val);
			}
			else if (String::equals(Keys::OptionsLaunchProfiler, key))
			{
				if (!m_inputs.launchProfiler().has_value())
					m_inputs.setLaunchProfiler(value.get<bool>());
			}
			else if (String::equals(Keys::OptionsKeepGoing, key))
			{
				if (!m_inputs.keepGoing().has_value())
					m_inputs.setKeepGoing(value.get<bool>());
			}
			else if (String::equals(Keys::OptionsCompilerCache, key))
			{
				if (!m_inputs.compilerCache().has_value())
					m_inputs.setCompilerCache(value.get<bool>());
			}
			else if (String::equals(Keys::OptionsGenerateCompileCommands, key))
			{
				if (!m_inputs.generateCompileCommands().has_value())
					m_inputs.setGenerateCompileCommands(value.get<bool>());
			}
			else if (String::equals(Keys::OptionsOnlyRequired, key))
			{
				if (!m_inputs.onlyRequired().has_value())
					m_inputs.setOnlyRequired(value.get<bool>());
			}
			else
				removeKeys.push_back(key);
		}
		else if (value.is_number())
		{
			if (String::equals(Keys::OptionsMaxJobs, key))
			{
				if (!m_inputs.maxJobs().has_value())
					m_inputs.setMaxJobs(static_cast<u32>(value.get<i32>()));
			}
			else
				removeKeys.push_back(key);
		}
		else if (value.is_object())
		{
			if (m_inputs.route().willRun() && String::equals(Keys::OptionsRunArguments, key))
			{
				Dictionary<StringList> map;
				for (const auto& [k, v] : value.items())
				{
					if (v.is_array())
					{
						map.emplace(k, v.get<StringList>());
					}
					else if (v.is_string())
					{
						// Note: if it's a string, it's from an old version of chalet, so don't read it
					}
				}
				m_centralState.setRunArgumentMap(std::move(map));
			}
		}
	}

	for (auto& key : removeKeys)
	{
		jOptions.erase(key);
	}

	if (!removeKeys.empty())
		return true;

	return false;
}

/*****************************************************************************/
bool SettingsJsonFile::readFromTools(Json& inNode)
{
	chalet_assert(json::isObject(inNode, Keys::Tools), "required node is invalid");

	auto& jTools = inNode[Keys::Tools];

	StringList removeKeys;
	for (const auto& [key, value] : jTools.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::ToolsBash, key))
				m_centralState.tools.setBash(value.get<std::string>());
			else if (String::equals(Keys::ToolsCcache, key))
				m_centralState.tools.setCcache(value.get<std::string>());
			else if (String::equals(Keys::ToolsCodesign, key))
				m_centralState.tools.setCodesign(value.get<std::string>());
			else if (String::equals(Keys::ToolsCommandPrompt, key))
				m_centralState.tools.setCommandPrompt(value.get<std::string>());
			else if (String::equals(Keys::ToolsCurl, key))
				m_centralState.tools.setCurl(value.get<std::string>());
			else if (String::equals(Keys::ToolsGit, key))
				m_centralState.tools.setGit(value.get<std::string>());
			else if (String::equals(Keys::ToolsHdiutil, key))
				m_centralState.tools.setHdiutil(value.get<std::string>());
			else if (String::equals(Keys::ToolsInstallNameTool, key))
				m_centralState.tools.setInstallNameTool(value.get<std::string>());
			else if (String::equals(Keys::ToolsInstruments, key))
				m_centralState.tools.setInstruments(value.get<std::string>());
			else if (String::equals(Keys::ToolsLdd, key))
				m_centralState.tools.setLdd(value.get<std::string>());
			else if (String::equals(Keys::ToolsOsascript, key))
				m_centralState.tools.setOsascript(value.get<std::string>());
			else if (String::equals(Keys::ToolsOtool, key))
				m_centralState.tools.setOtool(value.get<std::string>());
			else if (String::equals(Keys::ToolsPlutil, key))
				m_centralState.tools.setPlutil(value.get<std::string>());
			else if (String::equals(Keys::ToolsPowershell, key))
				m_centralState.tools.setPowershell(value.get<std::string>());
			else if (String::equals(Keys::ToolsSample, key))
				m_centralState.tools.setSample(value.get<std::string>());
			else if (String::equals(Keys::ToolsShasum, key))
				m_centralState.tools.setShasum(value.get<std::string>());
			else if (String::equals(Keys::ToolsSips, key))
				m_centralState.tools.setSips(value.get<std::string>());
			else if (String::equals(Keys::ToolsTar, key))
				m_centralState.tools.setTar(value.get<std::string>());
			else if (String::equals(Keys::ToolsTiffutil, key))
				m_centralState.tools.setTiffutil(value.get<std::string>());
			else if (String::equals(Keys::ToolsUnzip, key))
				m_centralState.tools.setUnzip(value.get<std::string>());
			else if (String::equals(Keys::ToolsXcodebuild, key))
				m_centralState.tools.setXcodebuild(value.get<std::string>());
			else if (String::equals(Keys::ToolsXcrun, key))
				m_centralState.tools.setXcrun(value.get<std::string>());
			else if (String::equals(Keys::ToolsZip, key))
				m_centralState.tools.setZip(value.get<std::string>());
			else
				removeKeys.push_back(key);
		}
	}

	for (auto& key : removeKeys)
	{
		jTools.erase(key);
	}

	if (!removeKeys.empty())
		return true;

	return false;
}

/*****************************************************************************/
// TODO: 'appleSdks' should become 'platformSDKs'
//
void SettingsJsonFile::readFromPlatformSDKs(Json& inNode)
{
#if defined(CHALET_MACOS)
	chalet_assert(json::isObject(inNode, Keys::AppleSdks), "required node is invalid");

	Json& appleSdks = inNode[Keys::AppleSdks];
	for (auto& [key, jPath] : appleSdks.items())
	{
		chalet_assert(jPath.is_string(), "sdk path should be a string by this point");
		m_centralState.tools.addPathToPlatformSDK(key, jPath.get<std::string>());
	}
#else
	UNUSED(inNode);
#endif
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool SettingsJsonFile::detectPathsToGitAndLLD(Json& jTools)
{
	bool dirty = false;
	auto gitPath = jTools[Keys::ToolsGit].get<std::string>();
	if (gitPath.empty())
	{
		gitPath = AncillaryTools::getPathToGit();
		jTools[Keys::ToolsGit] = gitPath;
		dirty = true;
	}
	else
	{
		if (!AncillaryTools::gitIsRootPath(gitPath))
		{
			jTools[Keys::ToolsGit] = gitPath;
			dirty = true;
		}
	}

	bool gitExists = Files::pathExists(gitPath);
	if (gitExists)
	{
		const auto gitBinFolder = String::getPathFolder(gitPath);
		const auto gitRoot = String::getPathFolder(gitBinFolder);

		auto& bashNode = jTools[Keys::ToolsBash];
		auto bashPath = bashNode.get<std::string>();

		// We need to ignore WSL bash
		if (!gitPath.empty() && (bashPath.empty() || String::contains("SYSTEM32", bashPath)))
		{
			bashPath = fmt::format("{}/bash.exe", gitBinFolder);
			if (Files::pathExists(bashPath))
			{
				jTools[Keys::ToolsBash] = bashPath;
				dirty = true;
			}
		}

		auto& lddNode = jTools[Keys::ToolsLdd];
		auto lddPath = lddNode.get<std::string>();
		if (lddPath.empty() && !gitPath.empty())
		{
			lddPath = fmt::format("{}/usr/bin/ldd.exe", gitRoot);
			if (Files::pathExists(lddPath))
			{
				jTools[Keys::ToolsLdd] = lddPath;
				dirty = true;
			}
		}
	}

	return dirty;
}

#elif defined(CHALET_MACOS)
/*****************************************************************************/
bool SettingsJsonFile::detectPathsToPlatformSDKs(Json& inNode, const bool inForce)
{
	// AppleTVOS.platform
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform

	chalet_assert(json::isObject(inNode, Keys::Tools), "required node is invalid");
	chalet_assert(json::isObject(inNode, Keys::AppleSdks), "required node is invalid");

	auto& jTools = inNode[Keys::Tools];
	auto& jAppleSDKs = inNode[Keys::AppleSdks];

	chalet_assert(jTools.contains(Keys::ToolsXcrun), "xcrun not found in tools structure");

	bool dirty = false;
	auto xcrun = jTools[Keys::ToolsXcrun].get<std::string>();
	auto sdkPaths = CompilerCxxAppleClang::getAllowedSDKTargets();
	for (const auto& sdk : sdkPaths)
	{
		if (inForce || !json::isValid<std::string>(jAppleSDKs, sdk.c_str()))
		{
			jAppleSDKs[sdk] = Process::runOutput({ xcrun, "--sdk", sdk, "--show-sdk-path" }, PipeOption::Pipe, PipeOption::Close);
			dirty = true;
		}
	}

	return dirty;
}
#endif

/*****************************************************************************/
bool SettingsJsonFile::validatePlatformSDKs(JsonFile& inJsonFile)
{
#if defined(CHALET_MACOS)
	auto sdkPath = m_centralState.tools.getPathToPlatformSDK("macosx");
	if (sdkPath.empty() || !Files::pathExists(sdkPath) || !Files::pathIsDirectory(sdkPath))
	{
	#if defined(CHALET_DEBUG)
		json::dump(inJsonFile.root[Keys::AppleSdks], 1, '\t');
		inJsonFile.dumpToTerminal();
	#endif

		detectPathsToPlatformSDKs(inJsonFile.root, true);
		readFromPlatformSDKs(inJsonFile.root);

		sdkPath = m_centralState.tools.getPathToPlatformSDK("macosx");
		if (sdkPath.empty() || !Files::pathExists(sdkPath) || !Files::pathIsDirectory(sdkPath))
		{
			Diagnostic::error("{}: The 'macosx' SDK path was not found, and one could not be detected.", inJsonFile.filename());
			return false;
		}

		// If the 2nd validation pass hasn't errored, force rebuild the project,
		//   since the sdks changed
		//
		m_centralState.cache.file().setForceRebuild(true);
	}
#else
	UNUSED(inJsonFile);
#endif

	return true;
}
}
