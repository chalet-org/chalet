/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonFile.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp" // getAllowedSDKTargets
#include "Core/CommandLineInputs.hpp"
#include "Platform/HostPlatform.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"

#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/CentralState.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
bool SettingsJsonFile::parseWithFallbackSettings(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback)
{
	SettingsJsonFile settingsJsonFile(inInputs, inCentralState, inFallback);
	return settingsJsonFile.deserialize();
}

/*****************************************************************************/
SettingsJsonFile::SettingsJsonFile(CommandLineInputs& inInputs, CentralState& inCentralState, const IntermediateSettingsState& inFallback) :
	m_inputs(inInputs),
	m_centralState(inCentralState),
	m_jsonFile(m_centralState.cache.getSettings(SettingsType::Local)),
	m_fallback(inFallback)
{}

/*****************************************************************************/
bool SettingsJsonFile::deserialize()
{
	Json schema = SettingsJsonSchema::get(m_inputs);
	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(schema, "schema/chalet-settings.schema.json");
	}

	/*bool cacheExists = m_centralState.cache.exists();
	if (cacheExists)
	{
		Diagnostic::infoEllipsis("Reading Settings [{}]", m_jsonFile.filename());
	}
	else
	{
		Diagnostic::infoEllipsis("Creating Settings [{}]", m_jsonFile.filename());
	}*/

	if (!makeSettingsJson())
		return false;

	if (!m_jsonFile.validate(schema))
		return false;

	if (!serializeFromJsonRoot(m_jsonFile.root))
	{
		Diagnostic::error("There was an error parsing {}", m_jsonFile.filename());
		return false;
	}

	if (!validatePaths(false))
		return false;

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::validatePaths(const bool inWithError)
{
#if defined(CHALET_MACOS)
	bool needsUpdate = false;
	auto sdkPaths = CompilerCxxAppleClang::getAllowedSDKTargets();
	// const bool commandLineTools = Files::isUsingAppleCommandLineTools();
	for (const auto& sdk : sdkPaths)
	{
		auto sdkPath = m_centralState.tools.getApplePlatformSdk(sdk);
		bool found = !sdkPath.empty() && Files::pathExists(sdkPath);
		// bool required = !found && (!commandLineTools || (commandLineTools && String::equals("macosx", sdk)));
		bool required = !found && String::equals("macosx", sdk);
		if (inWithError && required)
		{
			Diagnostic::error("{}: The '{}' SDK path was either not found or from a version of Xcode that has since been removed.", m_jsonFile.filename(), sdk);
			return false;
		}
		else if (!found && required)
		{
			needsUpdate = true;
			break;
		}
	}

	if (needsUpdate)
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif

		if (!detectAppleSdks(true))
			return false;

		if (!parseAppleSdks(m_jsonFile.root))
			return false;
	}
#endif

	if (!inWithError && !validatePaths(true))
		return false;

#if defined(CHALET_MACOS)
	if (needsUpdate)
	{
		// If the 2nd validation pass hasn't errored
		// force rebuild the project - the sdks changed
		//
		m_centralState.cache.file().setForceRebuild(true);
	}
#endif
	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::makeSettingsJson()
{
	// Create the json cache
	m_jsonFile.makeNode(Keys::Options, JsonDataType::object);

	auto& jRoot = m_jsonFile.root;
	if (!json::isObject(jRoot, Keys::Toolchains))
	{
		jRoot[Keys::Toolchains] = m_fallback.toolchains.is_object() ? m_fallback.toolchains : Json::object();
	}

	if (!json::isObject(jRoot, Keys::Tools))
	{
		jRoot[Keys::Tools] = m_fallback.tools.is_object() ? m_fallback.tools : Json::object();
	}
	else
	{
		if (m_fallback.tools.is_object())
		{
			auto& tools = jRoot[Keys::Tools];
			for (auto& [key, value] : m_fallback.tools.items())
			{
				if (!tools.contains(key))
				{
					tools[key] = value;
				}
			}
		}
	}

#if defined(CHALET_MACOS)
	if (!json::isObject(jRoot, Keys::AppleSdks))
	{
		jRoot[Keys::AppleSdks] = m_fallback.appleSdks.is_object() ? m_fallback.appleSdks : Json::object();
	}
	else
	{
		if (m_fallback.appleSdks.is_object())
		{
			auto& sdks = jRoot[Keys::AppleSdks];
			for (auto& [key, value] : m_fallback.appleSdks.items())
			{
				if (!sdks.contains(key))
				{
					sdks[key] = value;
				}
			}
		}
	}
#endif

	Json& buildOptions = jRoot[Keys::Options];

	{
		// pre 6.0.0
		const std::string kRunTarget{ "runTarget" };
		if (buildOptions.contains(kRunTarget))
		{
			buildOptions.erase(kRunTarget);
			m_jsonFile.setDirty(true);
		}
	}

	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsDumpAssembly, m_inputs.dumpAssembly(), m_fallback.dumpAssembly);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsShowCommands, m_inputs.showCommands(), m_fallback.showCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsBenchmark, m_inputs.benchmark(), m_fallback.benchmark);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsLaunchProfiler, m_inputs.launchProfiler(), m_fallback.launchProfiler);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsKeepGoing, m_inputs.keepGoing(), m_fallback.keepGoing);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsCompilerCache, m_inputs.compilerCache(), m_fallback.compilerCache);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsGenerateCompileCommands, m_inputs.generateCompileCommands(), m_fallback.generateCompileCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOnlyRequired, m_inputs.onlyRequired(), m_fallback.onlyRequired);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsMaxJobs, m_inputs.maxJobs(), m_fallback.maxJobs);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsToolchain, m_inputs.toolchainPreferenceName(), m_fallback.toolchainPreference);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsBuildConfiguration, m_inputs.buildConfiguration(), m_fallback.buildConfiguration);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsArchitecture, m_inputs.architectureRaw(), m_fallback.architecturePreference);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsInputFile, m_inputs.inputFile(), m_fallback.inputFile);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsEnvFile, m_inputs.envFile(), m_fallback.envFile);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsRootDirectory, m_inputs.rootDirectory(), m_fallback.rootDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOutputDirectory, m_inputs.outputDirectory(), m_fallback.outputDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsExternalDirectory, m_inputs.externalDirectory(), m_fallback.externalDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsDistributionDirectory, m_inputs.distributionDirectory(), m_fallback.distributionDirectory);

	// We always want to save these values
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOsTargetName, m_inputs.osTargetName(), m_fallback.osTargetName);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOsTargetVersion, m_inputs.osTargetVersion(), m_fallback.osTargetVersion);

	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsSigningIdentity, m_inputs.signingIdentity(), m_fallback.signingIdentity);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsProfilerConfig, m_inputs.profilerConfig(), m_fallback.profilerConfig);

	// if (!buildOptions.contains(Keys::OptionsSigningIdentity) || !buildOptions[Keys::OptionsSigningIdentity].is_string())
	// {
	// 	// Note: don't check if string is empty - empty is valid
	// 	buildOptions[Keys::OptionsSigningIdentity] = m_fallback.signingIdentity;
	// 	m_jsonFile.setDirty(true);
	// }

	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsLastTarget, m_inputs.lastTarget(), m_fallback.lastTarget);

	if (!json::isObject(buildOptions, Keys::OptionsRunArguments))
	{
		buildOptions[Keys::OptionsRunArguments] = Json::object();
		m_jsonFile.setDirty(true);
	}

	//

#if defined(CHALET_WIN32)
	HostPlatform platform = HostPlatform::Windows;
#elif defined(CHALET_MACOS)
	HostPlatform platform = HostPlatform::MacOS;
#else
	HostPlatform platform = HostPlatform::Linux;
#endif

	auto whichAdd = [this, &platform](Json& inNode, const std::string& inKey, const HostPlatform inPlatform = HostPlatform::Any) -> bool {
		if (!inNode.contains(inKey))
		{
			if (inPlatform == HostPlatform::Any || inPlatform == platform)
			{
				auto path = Files::which(inKey);
#if defined(CHALET_WIN32)
				String::replaceAll(path, "WINDOWS/SYSTEM32", "Windows/System32");
#endif
				bool res = !path.empty();
				inNode[inKey] = std::move(path);
				m_jsonFile.setDirty(true);
				return res;
			}
			else
			{
				inNode[inKey] = std::string();
				m_jsonFile.setDirty(true);
				return true;
			}
		}

		return true;
	};

	Json& tools = jRoot[Keys::Tools];

	whichAdd(tools, Keys::ToolsBash);
	whichAdd(tools, Keys::ToolsCcache);
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsCodesign, HostPlatform::MacOS);
#endif

#if defined(CHALET_WIN32)
	if (!tools.contains(Keys::ToolsCommandPrompt))
	{
		auto path = Files::which("cmd");
		String::replaceAll(path, "WINDOWS/SYSTEM32", "Windows/System32");
		tools[Keys::ToolsCommandPrompt] = std::move(path);
		m_jsonFile.setDirty(true);
	}
#endif
	whichAdd(tools, Keys::ToolsCurl);
	whichAdd(tools, Keys::ToolsGit);
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsHdiutil, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsInstallNameTool, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsInstruments, HostPlatform::MacOS);
#endif
	whichAdd(tools, Keys::ToolsLdd);

#if !defined(CHALET_WIN32)
	whichAdd(tools, Keys::ToolsShasum);
#endif
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsOsascript, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsOtool, HostPlatform::MacOS);
#endif
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsPlutil, HostPlatform::MacOS);
#endif

	if (!tools.contains(Keys::ToolsPowershell))
	{
		auto powershell = Files::which("pwsh"); // Powershell OS 6+ (ex: C:/Program Files/Powershell/6)
#if defined(CHALET_WIN32)
		if (powershell.empty())
			powershell = Files::which(Keys::ToolsPowershell);
#endif
		tools[Keys::ToolsPowershell] = std::move(powershell);
		m_jsonFile.setDirty(true);
	}

#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsSample, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsSips, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsTar);
	whichAdd(tools, Keys::ToolsTiffutil, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsXcodebuild, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsXcrun, HostPlatform::MacOS);
#else
	whichAdd(tools, Keys::ToolsTar);
#endif

#if !defined(CHALET_WIN32)
	whichAdd(tools, Keys::ToolsUnzip);
	whichAdd(tools, Keys::ToolsZip);
#endif

#if defined(CHALET_WIN32)
	{
		// Try really hard to find these tools

		auto& gitNode = tools[Keys::ToolsGit];
		auto gitPath = gitNode.get<std::string>();
		if (gitPath.empty())
		{
			gitPath = AncillaryTools::getPathToGit();
			tools[Keys::ToolsGit] = gitPath;
		}
		else
		{
			if (!AncillaryTools::gitIsRootPath(gitPath))
				tools[Keys::ToolsGit] = gitPath;
		}

		bool gitExists = Files::pathExists(gitPath);
		if (gitExists)
		{
			const auto gitBinFolder = String::getPathFolder(gitPath);
			const auto gitRoot = String::getPathFolder(gitBinFolder);

			auto& bashNode = tools[Keys::ToolsBash];
			auto bashPath = bashNode.get<std::string>();

			// We need to ignore WSL bash
			if (!gitPath.empty() && (bashPath.empty() || String::contains("SYSTEM32", bashPath)))
			{
				bashPath = fmt::format("{}/{}", gitBinFolder, "bash.exe");
				if (Files::pathExists(bashPath))
				{
					tools[Keys::ToolsBash] = bashPath;
				}
			}

			auto& lddNode = tools[Keys::ToolsLdd];
			auto lddPath = lddNode.get<std::string>();
			if (lddPath.empty() && !gitPath.empty())
			{
				lddPath = fmt::format("{}/usr/bin/{}", gitRoot, "ldd.exe");
				if (Files::pathExists(lddPath))
				{
					tools[Keys::ToolsLdd] = lddPath;
				}
			}
		}
	}
#endif

#if defined(CHALET_MACOS)
	if (!detectAppleSdks())
		return false;
#endif

	if (jRoot.contains(Keys::LastUpdateCheck))
		jRoot.erase(Keys::LastUpdateCheck);

	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::serializeFromJsonRoot(Json& inJson)
{
	if (!inJson.is_object())
	{
		Diagnostic::error("{}: Json root must be an object.", m_jsonFile.filename());
		return false;
	}

	if (!parseSettings(inJson))
		return false;

	if (!parseTools(inJson))
		return false;

	/*
	if (!inNode.contains(Keys::CompilerTools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::CompilerTools);
		return false;
	}

	Json& toolchains = inNode[Keys::CompilerTools];
	if (!toolchains.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::CompilerTools);
		return false;
	}
*/

#if defined(CHALET_MACOS)
	if (!parseAppleSdks(inJson))
		return false;
#endif
	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::parseSettings(Json& inNode)
{
	if (!inNode.contains(Keys::Options))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::Options);
		return false;
	}

	Json& buildOptions = inNode[Keys::Options];
	if (!buildOptions.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Options);
		return false;
	}

	StringList removeKeys;
	for (const auto& [key, value] : buildOptions.items())
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

					// Note: if it's a string, it's from an old version of chalet, so don't read it
				}
				m_centralState.setRunArgumentMap(std::move(map));
			}
		}
	}

	for (auto& key : removeKeys)
	{
		buildOptions.erase(key);
	}

	if (!removeKeys.empty())
		m_jsonFile.setDirty(true);

	return true;
}

/*****************************************************************************/
bool SettingsJsonFile::parseTools(Json& inNode)
{
	if (!inNode.contains(Keys::Tools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::Tools);
		return false;
	}

	Json& tools = inNode[Keys::Tools];
	if (!tools.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), Keys::Tools);
		return false;
	}

	StringList removeKeys;
	for (const auto& [key, value] : tools.items())
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
		tools.erase(key);
	}

	if (!removeKeys.empty())
		m_jsonFile.setDirty(true);

	return true;
}

#if defined(CHALET_MACOS)

/*****************************************************************************/
bool SettingsJsonFile::detectAppleSdks(const bool inForce)
{
	// AppleTVOS.platform
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	Json& appleSkdsJson = m_jsonFile.root[Keys::AppleSdks];

	chalet_assert(m_jsonFile.root.contains(Keys::Tools), "tools structure was not found");
	auto& tools = m_jsonFile.root[Keys::Tools];

	chalet_assert(tools.contains(Keys::ToolsXcrun), "xcrun not found in tools structure");

	auto xcrun = tools[Keys::ToolsXcrun].get<std::string>();
	auto sdkPaths = CompilerCxxAppleClang::getAllowedSDKTargets();
	for (const auto& sdk : sdkPaths)
	{
		if (inForce || !appleSkdsJson.contains(sdk))
		{
			appleSkdsJson[sdk] = Process::runOutput({ xcrun, "--sdk", sdk, "--show-sdk-path" }, PipeOption::Pipe, PipeOption::Close);
			m_jsonFile.setDirty(true);
		}
	}

	return true;
}
/*****************************************************************************/
bool SettingsJsonFile::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(Keys::AppleSdks))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::AppleSdks);
		return false;
	}

	Json& appleSdks = inNode[Keys::AppleSdks];
	for (auto& [key, pathJson] : appleSdks.items())
	{
		if (!pathJson.is_string())
		{
			Diagnostic::error("{}: apple platform '{}' must be a string.", m_jsonFile.filename(), key);
			return false;
		}

		auto path = pathJson.get<std::string>();
		m_centralState.tools.addApplePlatformSdk(key, std::move(path));
	}

	return true;
}
#endif

}
