/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonParser.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Core/HostPlatform.hpp"
#include "State/GlobalConfigState.hpp"

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
CacheJsonParser::CacheJsonParser(const CommandLineInputs& inInputs, StatePrototype& inPrototype, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_prototype(inPrototype),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool CacheJsonParser::serialize(const GlobalConfigState& inState)
{
	Json cacheJsonSchema = Schema::getCacheJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(cacheJsonSchema, "schema/chalet-cache.schema.json");
	}

	Timer timer;
	bool cacheExists = m_prototype.cache.exists();
	if (cacheExists)
	{
		Diagnostic::info(fmt::format("Reading Cache [{}]", m_jsonFile.filename()), false);
	}
	else
	{
		Diagnostic::info(fmt::format("Creating Cache [{}]", m_jsonFile.filename()), false);
	}

	// Note: this should never get cached locally
	if (!addGlobalSettings(inState))
		return false;

	if (!makeCache(inState))
		return false;

	if (!m_jsonFile.validate(std::move(cacheJsonSchema)))
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
bool CacheJsonParser::addGlobalSettings(const GlobalConfigState& inState)
{
	m_prototype.ancillaryTools.setCertSigningRequest(inState.certSigningRequest);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::validatePaths()
{
#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_prototype.ancillaryTools.applePlatformSdk("macosx")))
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
bool CacheJsonParser::makeCache(const GlobalConfigState& inState)
{
	// TODO: Copy from global cache. If one doesn't exist, do this

	// Create the json cache
	m_jsonFile.makeNode(kKeyWorkingDirectory, JsonDataType::string);
	m_jsonFile.makeNode(kKeySettings, JsonDataType::object);

	if (!m_jsonFile.json.contains(kKeyToolchains))
	{
		if (inState.toolchains.is_object())
		{
			m_jsonFile.json[kKeyToolchains] = inState.toolchains;
		}
		else
		{
			m_jsonFile.json[kKeyToolchains] = Json::object();
		}
	}

	m_jsonFile.makeNode(kKeyTools, JsonDataType::object);
	m_jsonFile.makeNode(kKeyApplePlatformSdks, JsonDataType::object);
	m_jsonFile.makeNode(kKeyExternalDependencies, JsonDataType::object);
	m_jsonFile.makeNode(kKeyData, JsonDataType::object);
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

	Json& settings = m_jsonFile.json[kKeySettings];

	if (!settings.contains(kKeyDumpAssembly) || !settings[kKeyDumpAssembly].is_boolean())
	{
		settings[kKeyDumpAssembly] = inState.dumpAssembly;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyMaxJobs) || !settings[kKeyMaxJobs].is_number_integer())
	{
		settings[kKeyMaxJobs] = inState.maxJobs;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyShowCommands) || !settings[kKeyShowCommands].is_boolean())
	{
		settings[kKeyShowCommands] = inState.showCommands;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyLastToolchain) || !settings[kKeyLastToolchain].is_string())
	{
		if (inState.toolchainPreference.empty())
		{
			m_inputs.detectToolchainPreference();
			settings[kKeyLastToolchain] = m_inputs.toolchainPreferenceRaw();
		}
		else
		{
			settings[kKeyLastToolchain] = inState.toolchainPreference;
		}
		m_jsonFile.setDirty(true);
	}
	else
	{
		const auto& prefFromInput = m_inputs.toolchainPreferenceRaw();
		auto value = settings[kKeyLastToolchain].get<std::string>();
		if (!prefFromInput.empty() && value != prefFromInput)
		{
			if (inState.toolchainPreference.empty())
				settings[kKeyLastToolchain] = prefFromInput;
			else
				settings[kKeyLastToolchain] = inState.toolchainPreference;
			m_jsonFile.setDirty(true);
		}
		else
		{
			if (prefFromInput.empty() && value.empty())
			{
				if (inState.toolchainPreference.empty())
				{
					m_inputs.detectToolchainPreference();
					settings[kKeyLastToolchain] = m_inputs.toolchainPreferenceRaw();
				}
				else
				{
					settings[kKeyLastToolchain] = inState.toolchainPreference;
				}
				m_jsonFile.setDirty(true);
			}
		}
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

	Json& ancillaryTools = m_jsonFile.json[kKeyTools];

	whichAdd(ancillaryTools, kKeyBash);
	whichAdd(ancillaryTools, kKeyBrew, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyCodesign, HostPlatform::MacOS);

	if (!ancillaryTools.contains(kKeyCommandPrompt))
	{
#if defined(CHALET_WIN32)
		auto res = Commands::which("cmd");
		String::replaceAll(res, "WINDOWS/SYSTEM32", "Windows/System32");
		ancillaryTools[kKeyCommandPrompt] = std::move(res);
#else
		ancillaryTools[kKeyCommandPrompt] = std::string();
#endif
		m_jsonFile.setDirty(true);
	}

	whichAdd(ancillaryTools, kKeyGit);
	whichAdd(ancillaryTools, kKeyGprof);
	whichAdd(ancillaryTools, kKeyHdiutil, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyInstallNameTool, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyInstruments, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyLdd);
	whichAdd(ancillaryTools, kKeyLipo, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyLua);

	whichAdd(ancillaryTools, kKeyOsascript, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyOtool, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyPerl);
	whichAdd(ancillaryTools, kKeyPlutil, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyPython);
	whichAdd(ancillaryTools, kKeyPython3);

	if (!ancillaryTools.contains(kKeyPowershell))
	{
		auto powershell = Commands::which("pwsh"); // Powershell OS 6+ (ex: C:/Program Files/Powershell/6)
#if defined(CHALET_WIN32)
		if (powershell.empty())
			powershell = Commands::which(kKeyPowershell);
#endif
		ancillaryTools[kKeyPowershell] = std::move(powershell);
		m_jsonFile.setDirty(true);
	}

	whichAdd(ancillaryTools, kKeyRuby);
	whichAdd(ancillaryTools, kKeySample, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeySips, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyTiffutil, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyXcodebuild, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyXcodegen, HostPlatform::MacOS);
	whichAdd(ancillaryTools, kKeyXcrun, HostPlatform::MacOS);

#if defined(CHALET_MACOS)
	// AppleTVOS.platform/
	// AppleTVSimulator.platform
	// MacOSX.platform
	// WatchOS.platform
	// WatchSimulator.platform
	// iPhoneOS.platform
	// iPhoneSimulator.platform
	Json& platformSdksJson = m_jsonFile.json[kKeyApplePlatformSdks];

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
		if (!platformSdksJson.contains(sdk))
		{
			std::string sdkPath = Commands::subprocessOutput({ "xcrun", "--sdk", sdk, "--show-sdk-path" });
			platformSdksJson[sdk] = std::move(sdkPath);
			m_jsonFile.setDirty(true);
		}
	}
#endif

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::serializeFromJsonRoot(Json& inJson)
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
bool CacheJsonParser::parseSettings(const Json& inNode)
{
	if (!inNode.contains(kKeySettings))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeySettings);
		return false;
	}

	const Json& settings = inNode.at(kKeySettings);
	if (!settings.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeySettings);
		return false;
	}

	if (bool val = false; m_jsonFile.assignFromKey(val, settings, kKeyShowCommands))
		Output::setShowCommands(val);

	if (bool val = false; m_jsonFile.assignFromKey(val, settings, kKeyDumpAssembly))
		m_prototype.environment.setDumpAssembly(val);

	if (ushort val = 0; m_jsonFile.assignFromKey(val, settings, kKeyMaxJobs))
		m_prototype.environment.setMaxJobs(val);

	if (std::string val; m_jsonFile.assignFromKey(val, settings, kKeyLastToolchain))
		m_inputs.setToolchainPreference(std::move(val));

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseTools(Json& inNode)
{
	if (!inNode.contains(kKeyTools))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	Json& ancillaryTools = inNode.at(kKeyTools);
	if (!ancillaryTools.is_object())
	{
		Diagnostic::error("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyTools);
		return false;
	}

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyBash))
		m_prototype.ancillaryTools.setBash(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyBrew))
		m_prototype.ancillaryTools.setBrew(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyCodesign))
		m_prototype.ancillaryTools.setCodesign(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyCommandPrompt))
		m_prototype.ancillaryTools.setCommandPrompt(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyGit))
		m_prototype.ancillaryTools.setGit(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyGprof))
		m_prototype.ancillaryTools.setGprof(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyHdiutil))
		m_prototype.ancillaryTools.setHdiutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyInstallNameTool))
		m_prototype.ancillaryTools.setInstallNameTool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyInstruments))
		m_prototype.ancillaryTools.setInstruments(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyLdd))
		m_prototype.ancillaryTools.setLdd(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyLipo))
		m_prototype.ancillaryTools.setLipo(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyLua))
		m_prototype.ancillaryTools.setLua(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyOsascript))
		m_prototype.ancillaryTools.setOsascript(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyOtool))
		m_prototype.ancillaryTools.setOtool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyPerl))
		m_prototype.ancillaryTools.setPerl(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyPlutil))
		m_prototype.ancillaryTools.setPlutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyPowershell))
		m_prototype.ancillaryTools.setPowershell(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyPython))
		m_prototype.ancillaryTools.setPython(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyPython3))
		m_prototype.ancillaryTools.setPython3(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyRuby))
		m_prototype.ancillaryTools.setRuby(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeySample))
		m_prototype.ancillaryTools.setSample(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeySips))
		m_prototype.ancillaryTools.setSips(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyTiffutil))
		m_prototype.ancillaryTools.setTiffutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyXcodebuild))
		m_prototype.ancillaryTools.setXcodebuild(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyXcodegen))
		m_prototype.ancillaryTools.setXcodegen(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, ancillaryTools, kKeyXcrun))
		m_prototype.ancillaryTools.setXcrun(std::move(val));

	return true;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
bool CacheJsonParser::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(kKeyApplePlatformSdks))
	{
		Diagnostic::error("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyApplePlatformSdks);
		return false;
	}

	Json& platformSdks = inNode.at(kKeyApplePlatformSdks);
	for (auto& [key, pathJson] : platformSdks.items())
	{
		if (!pathJson.is_string())
		{
			Diagnostic::error("{}: apple platform '{}' must be a string.", m_jsonFile.filename(), key);
			return false;
		}

		auto path = pathJson.get<std::string>();
		m_prototype.ancillaryTools.addApplePlatformSdk(key, std::move(path));
	}

	return true;
}
#endif

}
