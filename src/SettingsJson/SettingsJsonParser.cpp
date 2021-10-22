/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Core/HostPlatform.hpp"
#include "SettingsJson/GlobalSettingsState.hpp"
#include "SettingsJson/SchemaSettingsJson.hpp"

#include "State/BuildState.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

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
	Json schema = Schema::getSettingsJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(schema, "schema/chalet-settings.schema.json");
	}

	Timer timer;
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

	m_jsonFile.assignNodeIfEmpty<std::string>(buildSettings, kKeySigningIdentity, [&]() {
		return inState.signingIdentity;
	});

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
#if defined(CHALET_WIN32)
	whichAdd(tools, kKeyMakeNsis, HostPlatform::Windows);
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

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyDumpAssembly))
	{
		if (!m_inputs.dumpAssembly().has_value())
			m_inputs.setDumpAssembly(val);
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyShowCommands))
	{
		if (m_inputs.showCommands().has_value())
			val = *m_inputs.showCommands();

		Output::setShowCommands(val);
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyBenchmark))
	{
		if (m_inputs.benchmark().has_value())
			val = *m_inputs.benchmark();

		Output::setShowBenchmarks(val);
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyGenerateCompileCommands))
	{
		if (!m_inputs.generateCompileCommands().has_value())
			m_inputs.setGenerateCompileCommands(val);
	}

	if (ushort val = 0; m_jsonFile.assignFromKey(val, buildSettings, kKeyMaxJobs))
	{
		if (!m_inputs.maxJobs().has_value())
			m_inputs.setMaxJobs(val);
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastBuildConfiguration))
	{
		if (m_inputs.buildConfiguration().empty())
			m_inputs.setBuildConfiguration(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastToolchain))
	{
		if (m_inputs.toolchainPreferenceName().empty())
			m_inputs.setToolchainPreference(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastArchitecture))
	{
		if (m_inputs.architectureRaw().empty())
			m_inputs.setArchitectureRaw(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeySigningIdentity))
		m_prototype.tools.setSigningIdentity(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyInputFile))
	{
		if (m_inputs.inputFile().empty() || !String::equals({ m_inputs.inputFile(), m_inputs.defaultInputFile() }, val))
			m_inputs.setInputFile(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyEnvFile))
	{
		if (m_inputs.envFile().empty() || !String::equals({ m_inputs.envFile(), m_inputs.defaultEnvFile() }, val))
			m_inputs.setEnvFile(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyRootDirectory))
	{
		if (m_inputs.rootDirectory().empty() || !String::equals(m_inputs.rootDirectory(), val))
			m_inputs.setRootDirectory(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyOutputDirectory))
	{
		if (m_inputs.outputDirectory().empty() || !String::equals({ m_inputs.outputDirectory(), m_inputs.defaultOutputDirectory() }, val))
			m_inputs.setOutputDirectory(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyExternalDirectory))
	{
		if (m_inputs.externalDirectory().empty() || !String::equals({ m_inputs.externalDirectory(), m_inputs.defaultExternalDirectory() }, val))
			m_inputs.setExternalDirectory(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyDistributionDirectory))
	{
		if (m_inputs.distributionDirectory().empty() || !String::equals({ m_inputs.distributionDirectory(), m_inputs.defaultDistributionDirectory() }, val))
			m_inputs.setDistributionDirectory(std::move(val));
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

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyBash))
		m_prototype.tools.setBash(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyBrew))
		m_prototype.tools.setBrew(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyCodesign))
		m_prototype.tools.setCodesign(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyCommandPrompt))
		m_prototype.tools.setCommandPrompt(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyGit))
		m_prototype.tools.setGit(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyHdiutil))
		m_prototype.tools.setHdiutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstallNameTool))
		m_prototype.tools.setInstallNameTool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstruments))
		m_prototype.tools.setInstruments(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLdd))
		m_prototype.tools.setLdd(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLua))
		m_prototype.tools.setLua(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyMakeNsis))
		m_prototype.tools.setMakeNsis(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyOsascript))
		m_prototype.tools.setOsascript(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyOtool))
		m_prototype.tools.setOtool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPerl))
		m_prototype.tools.setPerl(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPlutil))
		m_prototype.tools.setPlutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPowershell))
		m_prototype.tools.setPowershell(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPython))
		m_prototype.tools.setPython(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPython3))
		m_prototype.tools.setPython3(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyRuby))
		m_prototype.tools.setRuby(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeySample))
		m_prototype.tools.setSample(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeySips))
		m_prototype.tools.setSips(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyTiffutil))
		m_prototype.tools.setTiffutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcodebuild))
		m_prototype.tools.setXcodebuild(std::move(val));

	// if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcodegen))
	// 	m_prototype.tools.setXcodegen(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcrun))
		m_prototype.tools.setXcrun(std::move(val));

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
