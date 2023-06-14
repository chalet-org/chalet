/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonParser.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp" // getAllowedSDKTargets
#include "Core/CommandLineInputs.hpp"
#include "Core/HostPlatform.hpp"
#include "SettingsJson/IntermediateSettingsState.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"

#include "State/CentralState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"
// #include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsJsonParser::SettingsJsonParser(CommandLineInputs& inInputs, CentralState& inCentralState, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_centralState(inCentralState),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool SettingsJsonParser::serialize(const IntermediateSettingsState& inState)
{
	// Timer timer;

	SettingsJsonSchema schemaBuilder;
	Json schema = schemaBuilder.get();

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

	if (!makeSettingsJson(inState))
		return false;

	if (!m_jsonFile.validate(std::move(schema)))
		return false;

	if (!serializeFromJsonRoot(m_jsonFile.json))
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
bool SettingsJsonParser::validatePaths(const bool inWithError)
{
#if defined(CHALET_MACOS)
	bool needsUpdate = false;
	auto&& [sdkPaths, commandLineTools] = getAppleSdks();
	for (const auto& sdk : sdkPaths)
	{
		auto sdkPath = m_centralState.tools.getApplePlatformSdk(sdk);
		bool found = !sdkPath.empty() && Commands::pathExists(sdkPath);
		// bool required = !found && (!commandLineTools || (commandLineTools && String::equals("macosx", sdk)));
		bool required = !found && String::equals("macosx", sdk);
		if (inWithError && required)
		{
			Diagnostic::error("{}: The '{}' SDK path was either not found or from a version of Xcode that has since been removed.", m_jsonFile.filename(), sdk);
			return false;
		}
		else if (!found)
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

		if (!parseAppleSdks(m_jsonFile.json))
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
bool SettingsJsonParser::makeSettingsJson(const IntermediateSettingsState& inState)
{
	// Create the json cache
	m_jsonFile.makeNode(Keys::Options, JsonDataType::object);

	if (!m_jsonFile.json.contains(Keys::Toolchains) || !m_jsonFile.json[Keys::Toolchains].is_object())
	{
		m_jsonFile.json[Keys::Toolchains] = inState.toolchains.is_object() ? inState.toolchains : Json::object();
	}

	if (!m_jsonFile.json.contains(Keys::Tools) || !m_jsonFile.json[Keys::Tools].is_object())
	{
		m_jsonFile.json[Keys::Tools] = inState.tools.is_object() ? inState.tools : Json::object();
	}
	else
	{
		if (inState.tools.is_object())
		{
			for (auto& [key, value] : inState.tools.items())
			{
				if (!m_jsonFile.json[Keys::Tools].contains(key))
				{
					m_jsonFile.json[Keys::Tools][key] = value;
				}
			}
		}
	}

#if defined(CHALET_MACOS)
	if (!m_jsonFile.json.contains(Keys::AppleSdks) || !m_jsonFile.json[Keys::AppleSdks].is_object())
	{
		m_jsonFile.json[Keys::AppleSdks] = inState.appleSdks.is_object() ? inState.appleSdks : Json::object();
	}
	else
	{
		if (inState.appleSdks.is_object())
		{
			for (auto& [key, value] : inState.appleSdks.items())
			{
				if (!m_jsonFile.json[Keys::AppleSdks].contains(key))
				{
					m_jsonFile.json[Keys::AppleSdks][key] = value;
				}
			}
		}
	}
#endif

	Json& buildOptions = m_jsonFile.json[Keys::Options];

	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsDumpAssembly, m_inputs.dumpAssembly(), inState.dumpAssembly);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsShowCommands, m_inputs.showCommands(), inState.showCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsBenchmark, m_inputs.benchmark(), inState.benchmark);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsLaunchProfiler, m_inputs.launchProfiler(), inState.launchProfiler);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsKeepGoing, m_inputs.keepGoing(), inState.keepGoing);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsGenerateCompileCommands, m_inputs.generateCompileCommands(), inState.generateCompileCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsMaxJobs, m_inputs.maxJobs(), inState.maxJobs);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsToolchain, m_inputs.toolchainPreferenceName(), inState.toolchainPreference);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsBuildConfiguration, m_inputs.buildConfiguration(), inState.buildConfiguration);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsArchitecture, m_inputs.architectureRaw(), inState.architecturePreference);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsInputFile, m_inputs.inputFile(), inState.inputFile);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsEnvFile, m_inputs.envFile(), inState.envFile);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsRootDirectory, m_inputs.rootDirectory(), inState.rootDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOutputDirectory, m_inputs.outputDirectory(), inState.outputDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsExternalDirectory, m_inputs.externalDirectory(), inState.externalDirectory);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsDistributionDirectory, m_inputs.distributionDirectory(), inState.distributionDirectory);

	// We always want to save these values
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOsTargetName, m_inputs.osTargetName(), inState.osTargetName);
	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsOsTargetVersion, m_inputs.osTargetVersion(), inState.osTargetVersion);
	if (buildOptions.at(Keys::OptionsOsTargetName).get<std::string>().empty() && !inState.osTargetName.empty())
	{
		buildOptions[Keys::OptionsOsTargetName] = inState.osTargetName;
		m_jsonFile.setDirty(true);
	}
	if (buildOptions.at(Keys::OptionsOsTargetVersion).get<std::string>().empty() && !inState.osTargetVersion.empty())
	{
		buildOptions[Keys::OptionsOsTargetVersion] = inState.osTargetVersion;
		m_jsonFile.setDirty(true);
	}

	m_jsonFile.assignNodeWithFallback(buildOptions, Keys::OptionsSigningIdentity, m_inputs.signingIdentity(), inState.signingIdentity);

	// if (!buildOptions.contains(Keys::OptionsSigningIdentity) || !buildOptions[Keys::OptionsSigningIdentity].is_string())
	// {
	// 	// Note: don't check if string is empty - empty is valid
	// 	buildOptions[Keys::OptionsSigningIdentity] = inState.signingIdentity;
	// 	m_jsonFile.setDirty(true);
	// }

	m_jsonFile.assignNodeIfEmptyWithFallback(buildOptions, Keys::OptionsRunTarget, m_inputs.runTarget(), inState.runTarget);

	if (!buildOptions.contains(Keys::OptionsRunArguments) || !buildOptions[Keys::OptionsRunArguments].is_object())
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

	auto whichAdd = [&](Json& inNode, const std::string& inKey, const HostPlatform inPlatform = HostPlatform::Any) -> bool {
		if (!inNode.contains(inKey))
		{
			if (inPlatform == HostPlatform::Any || inPlatform == platform)
			{
				auto path = Commands::which(inKey);
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

	Json& tools = m_jsonFile.json[Keys::Tools];

	whichAdd(tools, Keys::ToolsBash);
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsCodesign, HostPlatform::MacOS);
#endif

#if defined(CHALET_WIN32)
	if (!tools.contains(Keys::ToolsCommandPrompt))
	{
		auto res = Commands::which("cmd");
		String::replaceAll(res, "WINDOWS/SYSTEM32", "Windows/System32");
		tools[Keys::ToolsCommandPrompt] = std::move(res);
		m_jsonFile.setDirty(true);
	}
#endif

	whichAdd(tools, Keys::ToolsGit);
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsHdiutil, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsInstallNameTool, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsInstruments, HostPlatform::MacOS);
#endif
	whichAdd(tools, Keys::ToolsLdd);

#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsOsascript, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsOtool, HostPlatform::MacOS);
#endif
#if defined(CHALET_MACOS)
	whichAdd(tools, Keys::ToolsPlutil, HostPlatform::MacOS);
#endif

	if (!tools.contains(Keys::ToolsPowershell))
	{
		auto powershell = Commands::which("pwsh"); // Powershell OS 6+ (ex: C:/Program Files/Powershell/6)
#if defined(CHALET_WIN32)
		if (powershell.empty())
			powershell = Commands::which(Keys::ToolsPowershell);
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
	// whichAdd(tools, Keys::ToolsXcodegen, HostPlatform::MacOS);
	whichAdd(tools, Keys::ToolsXcrun, HostPlatform::MacOS);
#elif defined(CHALET_LINUX)
	whichAdd(tools, Keys::ToolsTar);
#endif

#if !defined(CHALET_WIN32)
	whichAdd(tools, Keys::ToolsZip);
#endif

#if defined(CHALET_WIN32)
	{
		// Try really hard to find these tools

		auto& gitNode = tools.at(Keys::ToolsGit);
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

		bool gitExists = Commands::pathExists(gitPath);
		if (gitExists)
		{
			const auto gitBinFolder = String::getPathFolder(gitPath);
			const auto gitRoot = String::getPathFolder(gitBinFolder);

			auto& bashNode = tools.at(Keys::ToolsBash);
			auto bashPath = bashNode.get<std::string>();
			if (bashPath.empty() && !gitPath.empty())
			{
				bashPath = fmt::format("{}/{}", gitBinFolder, "bash.exe");
				if (Commands::pathExists(bashPath))
				{
					tools[Keys::ToolsBash] = bashPath;
				}
			}

			auto& lddNode = tools.at(Keys::ToolsLdd);
			auto lddPath = lddNode.get<std::string>();
			if (lddPath.empty() && !gitPath.empty())
			{
				lddPath = fmt::format("{}/usr/bin/{}", gitRoot, "ldd.exe");
				if (Commands::pathExists(lddPath))
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

	if (m_jsonFile.json.contains(Keys::LastUpdateCheck))
	{
		m_jsonFile.json.erase(Keys::LastUpdateCheck);
	}

	return true;
}

/*****************************************************************************/
bool SettingsJsonParser::serializeFromJsonRoot(Json& inJson)
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

	Json& toolchains = inNode.at(Keys::CompilerTools);
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
bool SettingsJsonParser::parseSettings(Json& inNode)
{
	if (!inNode.contains(Keys::Options))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::Options);
		return false;
	}

	Json& buildOptions = inNode.at(Keys::Options);
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
			else if (String::equals(Keys::OptionsRunTarget, key))
			{
				if (m_inputs.runTarget().empty())
					m_inputs.setRunTarget(value.get<std::string>());
			}
			else if (String::equals(Keys::OptionsSigningIdentity, key))
			{
				if (m_inputs.signingIdentity().empty())
					m_inputs.setSigningIdentity(value.get<std::string>());
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
			else if (String::equals(Keys::OptionsGenerateCompileCommands, key))
			{
				if (!m_inputs.generateCompileCommands().has_value())
					m_inputs.setGenerateCompileCommands(value.get<bool>());
			}
			else
				removeKeys.push_back(key);
		}
		else if (value.is_number())
		{
			if (String::equals(Keys::OptionsMaxJobs, key))
			{
				if (!m_inputs.maxJobs().has_value())
					m_inputs.setMaxJobs(static_cast<uint>(value.get<int>()));
			}
			else
				removeKeys.push_back(key);
		}
		else if (value.is_object())
		{
			if (m_inputs.route().willRun() && String::equals(Keys::OptionsRunArguments, key))
			{
				Dictionary<std::string> map;
				for (const auto& [k, v] : value.items())
				{
					if (v.is_string())
						map.emplace(k, v.get<std::string>());
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
bool SettingsJsonParser::parseTools(Json& inNode)
{
	if (!inNode.contains(Keys::Tools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::Tools);
		return false;
	}

	Json& tools = inNode.at(Keys::Tools);
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
			else if (String::equals(Keys::ToolsCodesign, key))
				m_centralState.tools.setCodesign(value.get<std::string>());
			else if (String::equals(Keys::ToolsCommandPrompt, key))
				m_centralState.tools.setCommandPrompt(value.get<std::string>());
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
			else if (String::equals(Keys::ToolsSips, key))
				m_centralState.tools.setSips(value.get<std::string>());
			else if (String::equals(Keys::ToolsTar, key))
				m_centralState.tools.setTar(value.get<std::string>());
			else if (String::equals(Keys::ToolsTiffutil, key))
				m_centralState.tools.setTiffutil(value.get<std::string>());
			else if (String::equals(Keys::ToolsXcodebuild, key))
				m_centralState.tools.setXcodebuild(value.get<std::string>());
			// else if (String::equals(Keys::ToolsXcodegen, key))
			// 	m_centralState.tools.setXcodegen(value.get<std::string>());
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
std::pair<StringList, bool> SettingsJsonParser::getAppleSdks() const
{
	// AppleTVOS.platform
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	//
	const auto& xcodePath = Commands::getXcodePath();
	bool commandLineTools = String::startsWith("/Library/Developer/CommandLineTools", xcodePath);
	// clang-format off
	return std::make_pair(CompilerCxxAppleClang::getAllowedSDKTargets(), commandLineTools);
	// clang-format on
}
/*****************************************************************************/
bool SettingsJsonParser::detectAppleSdks(const bool inForce)
{
	// AppleTVOS.platform
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	Json& appleSkdsJson = m_jsonFile.json[Keys::AppleSdks];

	auto xcrun = Commands::which("xcrun");
	auto&& [sdkPaths, commandLineTools] = getAppleSdks();
	for (const auto& sdk : sdkPaths)
	{
		if (inForce || !appleSkdsJson.contains(sdk))
		{
			std::string sdkPath = Commands::subprocessOutput({ xcrun, "--sdk", sdk, "--show-sdk-path" }, PipeOption::Pipe, PipeOption::Close);
			if (!sdkPath.empty())
			{
				appleSkdsJson[sdk] = std::move(sdkPath);
				m_jsonFile.setDirty(true);
			}
		}
	}

	return true;
}
/*****************************************************************************/
bool SettingsJsonParser::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(Keys::AppleSdks))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), Keys::AppleSdks);
		return false;
	}

	Json& appleSdks = inNode.at(Keys::AppleSdks);
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
