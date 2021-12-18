/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Core/HostPlatform.hpp"
#include "SettingsJson/GlobalSettingsState.hpp"
#include "SettingsJson/SchemaSettingsJson.hpp"

#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
// #include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsJsonParser::SettingsJsonParser(CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool SettingsJsonParser::serialize(const GlobalSettingsState& inState)
{
	// Timer timer;

	SchemaSettingsJson schemaBuilder;
	Json schema = schemaBuilder.get();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(schema, "schema/chalet-settings.schema.json");
	}

	/*bool cacheExists = m_prototype.cache.exists();
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

	if (!validatePaths())
		return false;

	// Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
bool SettingsJsonParser::validatePaths()
{
#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_prototype.tools.applePlatformSdk("macosx")))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::error("{}: 'No MacOS SDK path could be found. Please install either Xcode or Command Line Tools.", m_jsonFile.filename());
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool SettingsJsonParser::makeSettingsJson(const GlobalSettingsState& inState)
{
	// TODO: Copy from global cache. If one doesn't exist, do this

	// Create the json cache
	m_jsonFile.makeNode(kKeyOptions, JsonDataType::object);

	if (!m_jsonFile.json.contains(kKeyToolchains) || !m_jsonFile.json[kKeyToolchains].is_object())
	{
		m_jsonFile.json[kKeyToolchains] = inState.toolchains.is_object() ? inState.toolchains : Json::object();
	}

	if (!m_jsonFile.json.contains(kKeyTools) || !m_jsonFile.json[kKeyTools].is_object())
	{
		m_jsonFile.json[kKeyTools] = inState.tools.is_object() ? inState.tools : Json::object();
	}
	else
	{
		if (inState.tools.is_object())
		{
			for (auto& [key, value] : inState.tools.items())
			{
				if (!m_jsonFile.json[kKeyTools].contains(key))
				{
					m_jsonFile.json[kKeyTools][key] = value;
				}
			}
		}
	}

#if defined(CHALET_MACOS)
	if (!m_jsonFile.json.contains(kKeyAppleSdks) || !m_jsonFile.json[kKeyAppleSdks].is_object())
	{
		m_jsonFile.json[kKeyAppleSdks] = inState.appleSdks.is_object() ? inState.appleSdks : Json::object();
	}
	else
	{
		if (inState.appleSdks.is_object())
		{
			for (auto& [key, value] : inState.appleSdks.items())
			{
				if (!m_jsonFile.json[kKeyAppleSdks].contains(key))
				{
					m_jsonFile.json[kKeyAppleSdks][key] = value;
				}
			}
		}
	}
#endif

	Json& buildSettings = m_jsonFile.json[kKeyOptions];

	m_jsonFile.assignNodeIfEmptyWithFallback<bool>(buildSettings, kKeyDumpAssembly, m_inputs.dumpAssembly(), inState.dumpAssembly);
	m_jsonFile.assignNodeIfEmptyWithFallback<bool>(buildSettings, kKeyShowCommands, m_inputs.showCommands(), inState.showCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback<bool>(buildSettings, kKeyBenchmark, m_inputs.benchmark(), inState.benchmark);
	m_jsonFile.assignNodeIfEmptyWithFallback<bool>(buildSettings, kKeyLaunchProfiler, m_inputs.launchProfiler(), inState.launchProfiler);
	m_jsonFile.assignNodeIfEmptyWithFallback<bool>(buildSettings, kKeyGenerateCompileCommands, m_inputs.generateCompileCommands(), inState.generateCompileCommands);
	m_jsonFile.assignNodeIfEmptyWithFallback<uint>(buildSettings, kKeyMaxJobs, m_inputs.maxJobs(), inState.maxJobs);

	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyLastToolchain, m_inputs.toolchainPreferenceName(), inState.toolchainPreference, [&]() {
		m_inputs.detectToolchainPreference();
	});

	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyLastBuildConfiguration, m_inputs.buildConfiguration(), inState.buildConfiguration);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyLastArchitecture, m_inputs.architectureRaw(), inState.architecturePreference);

	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyInputFile, m_inputs.inputFile(), inState.inputFile);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyEnvFile, m_inputs.envFile(), inState.envFile);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyRootDirectory, m_inputs.rootDirectory(), inState.rootDirectory);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyOutputDirectory, m_inputs.outputDirectory(), inState.outputDirectory);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyExternalDirectory, m_inputs.externalDirectory(), inState.externalDirectory);
	m_jsonFile.assignStringIfEmptyWithFallback(buildSettings, kKeyDistributionDirectory, m_inputs.distributionDirectory(), inState.distributionDirectory);

	if (!buildSettings.contains(kKeySigningIdentity) || !buildSettings[kKeySigningIdentity].is_string())
	{
		// Note: don't check if string is empty - empty is valid
		buildSettings[kKeySigningIdentity] = inState.signingIdentity;
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

	Json& tools = m_jsonFile.json[kKeyTools];

	whichAdd(tools, kKeyBash);
#if defined(CHALET_MACOS)
	whichAdd(tools, kKeyBrew, HostPlatform::MacOS);
	whichAdd(tools, kKeyCodesign, HostPlatform::MacOS);
#endif

#if defined(CHALET_WIN32)
	if (!tools.contains(kKeyCommandPrompt))
	{
		auto res = Commands::which("cmd");
		String::replaceAll(res, "WINDOWS/SYSTEM32", "Windows/System32");
		tools[kKeyCommandPrompt] = std::move(res);
		m_jsonFile.setDirty(true);
	}
#endif

	whichAdd(tools, kKeyGit);
#if defined(CHALET_MACOS)
	whichAdd(tools, kKeyHdiutil, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstallNameTool, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstruments, HostPlatform::MacOS);
#endif
	whichAdd(tools, kKeyLdd);
	whichAdd(tools, kKeyLua);
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	whichAdd(tools, kKeyMakeNsis);
#endif

#if defined(CHALET_MACOS)
	whichAdd(tools, kKeyOsascript, HostPlatform::MacOS);
	whichAdd(tools, kKeyOtool, HostPlatform::MacOS);
#endif
	whichAdd(tools, kKeyPerl);
#if defined(CHALET_MACOS)
	whichAdd(tools, kKeyPlutil, HostPlatform::MacOS);
#endif
	whichAdd(tools, kKeyPython);
	whichAdd(tools, kKeyPython3);

	if (!tools.contains(kKeyPowershell))
	{
		auto powershell = Commands::which("pwsh"); // Powershell OS 6+ (ex: C:/Program Files/Powershell/6)
#if defined(CHALET_WIN32)
		if (powershell.empty())
			powershell = Commands::which(kKeyPowershell);
#endif
		tools[kKeyPowershell] = std::move(powershell);
		m_jsonFile.setDirty(true);
	}

	whichAdd(tools, kKeyRuby);

#if defined(CHALET_MACOS)
	whichAdd(tools, kKeySample, HostPlatform::MacOS);
	whichAdd(tools, kKeySips, HostPlatform::MacOS);
	whichAdd(tools, kKeyTiffutil, HostPlatform::MacOS);
	whichAdd(tools, kKeyXcodebuild, HostPlatform::MacOS);
	// whichAdd(tools, kKeyXcodegen, HostPlatform::MacOS);
	whichAdd(tools, kKeyXcrun, HostPlatform::MacOS);
#endif

#if !defined(CHALET_WIN32)
	whichAdd(tools, kKeyZip);
#endif

#if defined(CHALET_WIN32)
	{
		// Try really hard to find these tools

		auto& gitNode = tools.at(kKeyGit);
		auto gitPath = gitNode.get<std::string>();
		if (gitPath.empty())
		{
			gitPath = m_prototype.tools.getPathToGit();
			tools[kKeyGit] = gitPath;
		}
		else
		{
			// We always want bin/git.exe (is not specific to cmd prompt or msys)
			if (String::contains("Git/mingw64/bin/git.exe", gitPath))
			{
				String::replaceAll(gitPath, "mingw64/", "");
				tools[kKeyGit] = gitPath;
			}
			else if (String::contains("Git/cmd/git.exe", gitPath))
			{
				String::replaceAll(gitPath, "cmd/", "bin");
				tools[kKeyGit] = gitPath;
			}
		}

		bool gitExists = Commands::pathExists(gitPath);
		if (gitExists)
		{
			const auto gitBinFolder = String::getPathFolder(gitPath);
			const auto gitRoot = String::getPathFolder(gitBinFolder);

			auto& bashNode = tools.at(kKeyBash);
			auto bashPath = bashNode.get<std::string>();
			if (bashPath.empty() && !gitPath.empty())
			{
				bashPath = fmt::format("{}/{}", gitBinFolder, "bash.exe");
				if (Commands::pathExists(bashPath))
				{
					tools[kKeyBash] = bashPath;
				}
			}

			auto& lddNode = tools.at(kKeyLdd);
			auto lddPath = lddNode.get<std::string>();
			if (lddPath.empty() && !gitPath.empty())
			{
				lddPath = fmt::format("{}/usr/bin/{}", gitRoot, "ldd.exe");
				if (Commands::pathExists(lddPath))
				{
					tools[kKeyLdd] = lddPath;
				}
			}

			// we can also do the same for perl
			auto& perlNode = tools.at(kKeyPerl);
			auto perlPath = perlNode.get<std::string>();
			if (perlPath.empty() && !gitPath.empty())
			{
				perlPath = fmt::format("{}/usr/bin/{}", gitRoot, "perl.exe");
				if (Commands::pathExists(perlPath))
				{
					tools[kKeyPerl] = perlPath;
				}
			}
		}
	}
#endif

#if defined(CHALET_MACOS)
	// AppleTVOS.platform/
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	Json& appleSkdsJson = m_jsonFile.json[kKeyAppleSdks];

	auto xcrun = Commands::which("xcrun");
	for (auto sdk : {
			 "appletvos",
			 "appletvsimulator",
			 "macosx",
			 "watchos",
			 "watchsimulator",
			 "iphoneos",
			 "iphonesimulator",
		 })
	{
		if (!appleSkdsJson.contains(sdk))
		{
			std::string sdkPath = Commands::subprocessOutput({ xcrun, "--sdk", sdk, "--show-sdk-path" });
			appleSkdsJson[sdk] = std::move(sdkPath);
			m_jsonFile.setDirty(true);
		}
	}
#endif

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
	if (!inNode.contains(kKeyCompilerTools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyCompilerTools);
		return false;
	}

	Json& toolchains = inNode.at(kKeyCompilerTools);
	if (!toolchains.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyCompilerTools);
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
bool SettingsJsonParser::parseSettings(const Json& inNode)
{
	if (!inNode.contains(kKeyOptions))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyOptions);
		return false;
	}

	const Json& buildSettings = inNode.at(kKeyOptions);
	if (!buildSettings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyOptions);
		return false;
	}

	for (const auto& [key, value] : buildSettings.items())
	{
		if (value.is_string())
		{
			if (String::equals(kKeyLastBuildConfiguration, key))
			{
				if (m_inputs.buildConfiguration().empty())
					m_inputs.setBuildConfiguration(value.get<std::string>());
			}
			else if (String::equals(kKeyLastToolchain, key))
			{
				if (m_inputs.toolchainPreferenceName().empty())
					m_inputs.setToolchainPreference(value.get<std::string>());
			}
			else if (String::equals(kKeyLastArchitecture, key))
			{
				if (m_inputs.architectureRaw().empty())
					m_inputs.setArchitectureRaw(value.get<std::string>());
			}
			else if (String::equals(kKeySigningIdentity, key))
			{
				m_prototype.tools.setSigningIdentity(value.get<std::string>());
			}
			else if (String::equals(kKeyInputFile, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.inputFile().empty() || !String::equals({ m_inputs.inputFile(), m_inputs.defaultInputFile() }, val))
					m_inputs.setInputFile(std::move(val));
			}
			else if (String::equals(kKeyEnvFile, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.envFile().empty() || !String::equals({ m_inputs.envFile(), m_inputs.defaultEnvFile() }, val))
					m_inputs.setEnvFile(std::move(val));
			}
			else if (String::equals(kKeyRootDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.rootDirectory().empty() || !String::equals(m_inputs.rootDirectory(), val))
					m_inputs.setRootDirectory(std::move(val));
			}
			else if (String::equals(kKeyOutputDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.outputDirectory().empty() || !String::equals({ m_inputs.outputDirectory(), m_inputs.defaultOutputDirectory() }, val))
					m_inputs.setOutputDirectory(std::move(val));
			}
			else if (String::equals(kKeyExternalDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.externalDirectory().empty() || !String::equals({ m_inputs.externalDirectory(), m_inputs.defaultExternalDirectory() }, val))
					m_inputs.setExternalDirectory(std::move(val));
			}
			else if (String::equals(kKeyDistributionDirectory, key))
			{
				auto val = value.get<std::string>();
				if (m_inputs.distributionDirectory().empty() || !String::equals({ m_inputs.distributionDirectory(), m_inputs.defaultDistributionDirectory() }, val))
					m_inputs.setDistributionDirectory(std::move(val));
			}
		}
		else if (value.is_boolean())
		{
			if (String::equals(kKeyDumpAssembly, key))
			{
				if (!m_inputs.dumpAssembly().has_value())
					m_inputs.setDumpAssembly(value.get<bool>());
			}
			else if (String::equals(kKeyShowCommands, key))
			{
				bool val = value.get<bool>();

				if (m_inputs.showCommands().has_value())
					val = *m_inputs.showCommands();

				Output::setShowCommands(val);
			}
			else if (String::equals(kKeyBenchmark, key))
			{
				bool val = value.get<bool>();

				if (m_inputs.benchmark().has_value())
					val = *m_inputs.benchmark();

				Output::setShowBenchmarks(val);
			}
			else if (String::equals(kKeyLaunchProfiler, key))
			{
				if (!m_inputs.launchProfiler().has_value())
					m_inputs.setLaunchProfiler(value.get<bool>());
			}
			else if (String::equals(kKeyLaunchProfiler, key))
			{
				if (!m_inputs.generateCompileCommands().has_value())
					m_inputs.setGenerateCompileCommands(value.get<bool>());
			}
		}
		else if (value.is_number())
		{
			if (String::equals(kKeyMaxJobs, key))
			{
				if (!m_inputs.maxJobs().has_value())
					m_inputs.setMaxJobs(static_cast<uint>(value.get<int>()));
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool SettingsJsonParser::parseTools(Json& inNode)
{
	if (!inNode.contains(kKeyTools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	Json& tools = inNode.at(kKeyTools);
	if (!tools.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	for (const auto& [key, value] : tools.items())
	{
		if (value.is_string())
		{
			if (String::equals(kKeyBash, key))
				m_prototype.tools.setBash(value.get<std::string>());
			else if (String::equals(kKeyBrew, key))
				m_prototype.tools.setBrew(value.get<std::string>());
			else if (String::equals(kKeyCodesign, key))
				m_prototype.tools.setCodesign(value.get<std::string>());
			else if (String::equals(kKeyCommandPrompt, key))
				m_prototype.tools.setCommandPrompt(value.get<std::string>());
			else if (String::equals(kKeyGit, key))
				m_prototype.tools.setGit(value.get<std::string>());
			else if (String::equals(kKeyHdiutil, key))
				m_prototype.tools.setHdiutil(value.get<std::string>());
			else if (String::equals(kKeyInstallNameTool, key))
				m_prototype.tools.setInstallNameTool(value.get<std::string>());
			else if (String::equals(kKeyInstruments, key))
				m_prototype.tools.setInstruments(value.get<std::string>());
			else if (String::equals(kKeyLdd, key))
				m_prototype.tools.setLdd(value.get<std::string>());
			else if (String::equals(kKeyLua, key))
				m_prototype.tools.setLua(value.get<std::string>());
			else if (String::equals(kKeyMakeNsis, key))
				m_prototype.tools.setMakeNsis(value.get<std::string>());
			else if (String::equals(kKeyOsascript, key))
				m_prototype.tools.setOsascript(value.get<std::string>());
			else if (String::equals(kKeyOtool, key))
				m_prototype.tools.setOtool(value.get<std::string>());
			else if (String::equals(kKeyPerl, key))
				m_prototype.tools.setPerl(value.get<std::string>());

			else if (String::equals(kKeyPlutil, key))
				m_prototype.tools.setPlutil(value.get<std::string>());
			else if (String::equals(kKeyPowershell, key))
				m_prototype.tools.setPowershell(value.get<std::string>());
			else if (String::equals(kKeyPython, key))
				m_prototype.tools.setPython(value.get<std::string>());
			else if (String::equals(kKeyPython3, key))
				m_prototype.tools.setPython3(value.get<std::string>());
			else if (String::equals(kKeyRuby, key))
				m_prototype.tools.setRuby(value.get<std::string>());
			else if (String::equals(kKeySample, key))
				m_prototype.tools.setSample(value.get<std::string>());
			else if (String::equals(kKeySips, key))
				m_prototype.tools.setSips(value.get<std::string>());
			else if (String::equals(kKeyTiffutil, key))
				m_prototype.tools.setTiffutil(value.get<std::string>());
			else if (String::equals(kKeyXcodebuild, key))
				m_prototype.tools.setXcodebuild(value.get<std::string>());
			// else if (String::equals(kKeyXcodegen, key))
			// 	m_prototype.tools.setXcodegen(value.get<std::string>());
			else if (String::equals(kKeyXcrun, key))
				m_prototype.tools.setXcrun(value.get<std::string>());
			else if (String::equals(kKeyZip, key))
				m_prototype.tools.setZip(value.get<std::string>());
		}
	}

	return true;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
bool SettingsJsonParser::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(kKeyAppleSdks))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyAppleSdks);
		return false;
	}

	Json& appleSdks = inNode.at(kKeyAppleSdks);
	for (auto& [key, pathJson] : appleSdks.items())
	{
		if (!pathJson.is_string())
		{
			Diagnostic::error("{}: apple platform '{}' must be a string.", m_jsonFile.filename(), key);
			return false;
		}

		auto path = pathJson.get<std::string>();
		m_prototype.tools.addApplePlatformSdk(key, std::move(path));
	}

	return true;
}
#endif

}
