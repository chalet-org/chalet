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
SettingsJsonParser::SettingsJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile) :
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
	bool cacheExists = m_prototype.cache.exists();
	if (cacheExists)
	{
		Diagnostic::info(fmt::format("Reading Settings [{}]", m_jsonFile.filename()), false);
	}
	else
	{
		Diagnostic::info(fmt::format("Creating Settings [{}]", m_jsonFile.filename()), false);
	}

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

	Diagnostic::printDone(timer.asString());

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
	m_jsonFile.makeNode(kKeyWorkingDirectory, JsonDataType::string);
	m_jsonFile.makeNode(kKeyBuild, JsonDataType::object);

	if (!m_jsonFile.json.contains(kKeyToolchains))
	{
		m_jsonFile.json[kKeyToolchains] = inState.toolchains.is_object() ? inState.toolchains : Json::object();
	}

	if (!m_jsonFile.json.contains(kKeyTools))
	{
		m_jsonFile.json[kKeyTools] = inState.tools.is_object() ? inState.tools : Json::object();
	}

	if (!m_jsonFile.json.contains(kKeyAppleSdks))
	{
		m_jsonFile.json[kKeyAppleSdks] = inState.appleSdks.is_object() ? inState.appleSdks : Json::object();
	}

	{
		Json& workingDirectoryJson = m_jsonFile.json[kKeyWorkingDirectory];
		const auto workingDirectory = workingDirectoryJson.get<std::string>();

		if (workingDirectory.empty())
		{
			auto tmp = Commands::getWorkingDirectory();
			Path::sanitize(tmp, true);
			workingDirectoryJson = std::move(tmp);
		}
	}

	Json& buildSettings = m_jsonFile.json[kKeyBuild];

	if (!buildSettings.contains(kKeyDumpAssembly) || !buildSettings[kKeyDumpAssembly].is_boolean())
	{
		buildSettings[kKeyDumpAssembly] = inState.dumpAssembly;
		m_jsonFile.setDirty(true);
	}

	if (!buildSettings.contains(kKeyMaxJobs) || !buildSettings[kKeyMaxJobs].is_number_integer())
	{
		buildSettings[kKeyMaxJobs] = inState.maxJobs;
		m_jsonFile.setDirty(true);
	}

	if (!buildSettings.contains(kKeyShowCommands) || !buildSettings[kKeyShowCommands].is_boolean())
	{
		buildSettings[kKeyShowCommands] = inState.showCommands;
		m_jsonFile.setDirty(true);
	}

	if (!buildSettings.contains(kKeyLastToolchain) || !buildSettings[kKeyLastToolchain].is_string())
	{
		if (inState.toolchainPreference.empty())
		{
			m_inputs.detectToolchainPreference();
			buildSettings[kKeyLastToolchain] = m_inputs.toolchainPreferenceRaw();
		}
		else
		{
			buildSettings[kKeyLastToolchain] = inState.toolchainPreference;
		}
		m_jsonFile.setDirty(true);
	}
	else
	{
		const auto& prefFromInput = m_inputs.toolchainPreferenceRaw();
		auto value = buildSettings[kKeyLastToolchain].get<std::string>();
		if (!prefFromInput.empty() && value != prefFromInput)
		{
			if (inState.toolchainPreference.empty())
				buildSettings[kKeyLastToolchain] = prefFromInput;
			else
				buildSettings[kKeyLastToolchain] = inState.toolchainPreference;
			m_jsonFile.setDirty(true);
		}
		else
		{
			if (prefFromInput.empty() && value.empty())
			{
				if (inState.toolchainPreference.empty())
				{
					m_inputs.detectToolchainPreference();
					buildSettings[kKeyLastToolchain] = m_inputs.toolchainPreferenceRaw();
				}
				else
				{
					buildSettings[kKeyLastToolchain] = inState.toolchainPreference;
				}
				m_jsonFile.setDirty(true);
			}
		}
	}

	if (!buildSettings.contains(kKeyMacosSigningIdentity) || !buildSettings[kKeyMacosSigningIdentity].is_string())
	{
		buildSettings[kKeyMacosSigningIdentity] = inState.macosSigningIdentity;
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

	Json& tools = m_jsonFile.json[kKeyTools];

	whichAdd(tools, kKeyBash);
	whichAdd(tools, kKeyBrew, HostPlatform::MacOS);
	whichAdd(tools, kKeyCodesign, HostPlatform::MacOS);

	if (!tools.contains(kKeyCommandPrompt))
	{
#if defined(CHALET_WIN32)
		auto res = Commands::which("cmd");
		String::replaceAll(res, "WINDOWS/SYSTEM32", "Windows/System32");
		tools[kKeyCommandPrompt] = std::move(res);
#else
		tools[kKeyCommandPrompt] = std::string();
#endif
		m_jsonFile.setDirty(true);
	}

	whichAdd(tools, kKeyGit);
	whichAdd(tools, kKeyGprof);
	whichAdd(tools, kKeyHdiutil, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstallNameTool, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstruments, HostPlatform::MacOS);
	whichAdd(tools, kKeyLdd);
	whichAdd(tools, kKeyLipo, HostPlatform::MacOS);
	whichAdd(tools, kKeyLua);

	whichAdd(tools, kKeyOsascript, HostPlatform::MacOS);
	whichAdd(tools, kKeyOtool, HostPlatform::MacOS);
	whichAdd(tools, kKeyPerl);
	whichAdd(tools, kKeyPlutil, HostPlatform::MacOS);
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
	whichAdd(tools, kKeySample, HostPlatform::MacOS);
	whichAdd(tools, kKeySips, HostPlatform::MacOS);
	whichAdd(tools, kKeyTiffutil, HostPlatform::MacOS);
	whichAdd(tools, kKeyXcodebuild, HostPlatform::MacOS);
	whichAdd(tools, kKeyXcodegen, HostPlatform::MacOS);
	whichAdd(tools, kKeyXcrun, HostPlatform::MacOS);

#if defined(CHALET_MACOS)
	// AppleTVOS.platform/
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	Json& appleSkdsJson = m_jsonFile.json[kKeyAppleSdks];

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
			std::string sdkPath = Commands::subprocessOutput({ "xcrun", "--sdk", sdk, "--show-sdk-path" });
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
	if (!inNode.contains(kKeyBuild))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyBuild);
		return false;
	}

	const Json& buildSettings = inNode.at(kKeyBuild);
	if (!buildSettings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyBuild);
		return false;
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyShowCommands))
		Output::setShowCommands(val);

	if (bool val = false; m_jsonFile.assignFromKey(val, buildSettings, kKeyDumpAssembly))
		m_prototype.environment.setDumpAssembly(val);

	if (ushort val = 0; m_jsonFile.assignFromKey(val, buildSettings, kKeyMaxJobs))
		m_prototype.environment.setMaxJobs(val);

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyLastToolchain))
		m_inputs.setToolchainPreference(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, buildSettings, kKeyMacosSigningIdentity))
		m_prototype.tools.setMacosSigningIdentity(std::move(val));

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

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyGprof))
		m_prototype.tools.setGprof(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyHdiutil))
		m_prototype.tools.setHdiutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstallNameTool))
		m_prototype.tools.setInstallNameTool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstruments))
		m_prototype.tools.setInstruments(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLdd))
		m_prototype.tools.setLdd(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLipo))
		m_prototype.tools.setLipo(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLua))
		m_prototype.tools.setLua(std::move(val));

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

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcodegen))
		m_prototype.tools.setXcodegen(std::move(val));

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
