/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonParser.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Core/HostPlatform.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
CacheJsonParser::CacheJsonParser(const CommandLineInputs& inInputs, BuildState& inState, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_state(inState),
	m_jsonFile(inJsonFile)
{
	UNUSED(m_state);
}

/*****************************************************************************/
bool CacheJsonParser::serialize()
{
#if defined(CHALET_WIN32)
	if (!createMsvcEnvironment())
		return false;
#endif

	Json cacheJsonSchema = Schema::getCacheJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(cacheJsonSchema, "schema/chalet-cache.schema.json");
	}

	Timer timer;
	bool cacheExists = m_state.cache.exists();
	if (cacheExists)
	{
		Diagnostic::info(fmt::format("Reading Cache [{}]", m_jsonFile.filename()), false);
	}
	else
	{
		Diagnostic::info(fmt::format("Creating Cache [{}]", m_jsonFile.filename()), false);
	}

	if (!makeCache())
		return false;

	if (!m_jsonFile.validate(std::move(cacheJsonSchema)))
		return false;

	if (!serializeFromJsonRoot(m_jsonFile.json))
	{
		Diagnostic::error(fmt::format("There was an error parsing {}", m_jsonFile.filename()));
		return false;
	}

	Diagnostic::printDone(timer.asString());

	if (!validatePaths())
		return false;

	if (!setDefaultBuildStrategy())
		return false;

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::createMsvcEnvironment()
{
#if defined(CHALET_WIN32)
	bool readVariables = true;

	const auto& jRoot = m_jsonFile.json;
	if (jRoot.is_object())
	{
		if (jRoot.contains(kKeyCompilerTools))
		{
			auto& compilerTools = jRoot[kKeyCompilerTools];
			if (compilerTools.is_object())
			{
				auto keyEndsWith = [&compilerTools](const std::string& inKey, std::string_view inExt) {
					if (compilerTools.contains(inKey))
					{
						const Json& j = compilerTools[inKey];
						const auto str = j.get<std::string>();
						return String::endsWith(inExt, str);
					}

					return false;
				};

				auto& toolchain = m_inputs.toolchainPreference();

				bool result = toolchain.type == ToolchainType::MSVC;
				if (!result)
				{
					result |= keyEndsWith(kKeyCpp, "cl.exe");
					result |= keyEndsWith(kKeyCc, "cl.exe");
					result |= keyEndsWith(kKeyLinker, "link.exe");
					result |= keyEndsWith(kKeyArchiver, "lib.exe");
					result |= keyEndsWith(kKeyWindowsResource, "rc.exe");
				}
				readVariables = result;
			}
		}
	}

	if (readVariables)
	{
		return m_state.msvcEnvironment.readCompilerVariables();
	}
#endif

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::validatePaths()
{
	auto& compilerTools = m_state.compilerTools;
	if (!Commands::pathExists(compilerTools.cpp()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(compilerTools.cc()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C compiler was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(compilerTools.archiver()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's archive utility was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(compilerTools.linker()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's linker was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

#if defined(CHALET_WIN32)
	if (!Commands::pathExists(compilerTools.rc()))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::warn(fmt::format("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename()));
	}
#endif

	UNUSED(m_make);
	/*
	if (!Commands::pathExists(m_make))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: 'make' could not be found.", m_jsonFile.filename()));
		return false;
	}
	*/

#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_state.tools.applePlatformSdk("macosx")))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::error(fmt::format("{}: 'No MacOS SDK path could be found. Please install either Xcode or Command Line Tools.", m_jsonFile.filename()));
		return false;
	}
#endif

	m_state.compilerTools.detectToolchain();

	if (m_state.compilerTools.detectedToolchain() == ToolchainType::LLVM)
	{
		if (m_inputs.targetArchitecture().empty())
		{
			// also takes -dumpmachine
			auto arch = Commands::subprocessOutput({ m_state.compilerTools.compiler(), "-print-target-triple" });
			// Strip out version in auto-detected mac triple
			auto isDarwin = arch.find("apple-darwin");
			if (isDarwin != std::string::npos)
			{
				arch = arch.substr(0, isDarwin + 12);
			}
			m_state.info.setTargetArchitecture(arch);
		}
	}
	else if (m_state.compilerTools.detectedToolchain() == ToolchainType::GNU)
	{
		auto arch = Commands::subprocessOutput({ m_state.compilerTools.compiler(), "-dumpmachine" });
		m_state.info.setTargetArchitecture(arch);
	}
#if defined(CHALET_WIN32)
	else if (m_state.compilerTools.detectedToolchain() == ToolchainType::MSVC)
	{
		const auto arch = m_state.info.targetArchitecture();
		switch (arch)
		{
			case Arch::Cpu::X64:
				m_state.info.setTargetArchitecture("x86_64-pc-windows-msvc");
				break;

			case Arch::Cpu::X86:
				m_state.info.setTargetArchitecture("i686-pc-windows-msvc");
				break;

			case Arch::Cpu::ARM:
			case Arch::Cpu::ARM64:
			default:
				break;
		}
	}
#endif

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::setDefaultBuildStrategy()
{
#if defined(CHALET_WIN32)
	auto& toolchain = m_inputs.toolchainPreference();
	bool usingMsvc = toolchain.type == ToolchainType::MSVC;
	if (!usingMsvc)
	{
		usingMsvc |= String::endsWith("cl.exe", m_state.compilerTools.cc());
		usingMsvc |= String::endsWith("cl.exe", m_state.compilerTools.cpp());
		usingMsvc |= String::endsWith("link.exe", m_state.compilerTools.linker());
		usingMsvc |= String::endsWith("lib.exe", m_state.compilerTools.archiver());
		usingMsvc |= String::endsWith("rc.exe", m_state.compilerTools.rc());
	}
	if (!usingMsvc)
	{
		m_state.msvcEnvironment.cleanup();
	}
#endif

	Json& settings = m_jsonFile.json[kKeySettings];
	Json& strategyJson = settings[kKeyStrategy];

	if (strategyJson.get<std::string>().empty() || m_changeStrategy)
	{
		if (Environment::isContinuousIntegrationServer())
		{
			strategyJson = "native-experimental";
		}
#if defined(CHALET_WIN32)
		else if (usingMsvc && !m_state.tools.ninja().empty())
		{
			strategyJson = "ninja";
		}
#endif
		else
		{
			strategyJson = "makefile";
		}

		const auto strategy = strategyJson.get<std::string>();
		m_state.environment.setStrategy(strategy);

		m_jsonFile.setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::makeToolchain(Json& compilerTools, const ToolchainPreference& toolchain)
{
	bool result = true;

	std::string cpp;
	std::string cc;
	if (!compilerTools.contains(kKeyCpp))
	{
		// auto varCXX = Environment::get("CXX");
		// if (varCXX != nullptr)
		// 	cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
			cpp = Commands::which(toolchain.cpp);
		}

		parseArchitecture(cpp);
		result &= !cpp.empty();

		compilerTools[kKeyCpp] = cpp;
		m_jsonFile.setDirty(true);
	}

	if (!compilerTools.contains(kKeyCc))
	{
		// auto varCC = Environment::get("CC");
		// if (varCC != nullptr)
		// 	cc = Commands::which(varCC);

		if (cc.empty())
		{
			cc = Commands::which(toolchain.cc);
		}

		parseArchitecture(cc);
		result &= !cc.empty();

		compilerTools[kKeyCc] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!compilerTools.contains(kKeyLinker))
	{
		std::string link;
		StringList linkers;
		linkers.push_back(toolchain.linker);
		if (toolchain.type == ToolchainType::LLVM)
		{
			linkers.push_back("ld");
		}

		for (const auto& linker : linkers)
		{
			link = Commands::which(linker);
			if (!link.empty())
				break;
		}

		// handles edge case w/ MSVC & MinGW in same path
		if (toolchain.type == ToolchainType::MSVC)
		{
			if (String::contains("/usr/bin/link", link))
			{
				if (!cc.empty())
					link = cc;
				else if (!cpp.empty())
					link = cpp;

				String::replaceAll(link, "cl.exe", "link.exe");
			}
		}

		parseArchitecture(link);
		result &= !link.empty();

		compilerTools[kKeyLinker] = std::move(link);
		m_jsonFile.setDirty(true);
	}

	if (!compilerTools.contains(kKeyArchiver))
	{
		std::string ar;
		StringList archivers;
		if (toolchain.type == ToolchainType::LLVM || toolchain.type == ToolchainType::GNU)
		{
			archivers.push_back("libtool");
		}
		archivers.push_back(toolchain.archiver);

		for (const auto& archiver : archivers)
		{
			ar = Commands::which(archiver);
			if (!ar.empty())
				break;
		}

		parseArchitecture(ar);
		result &= !ar.empty();

		compilerTools[kKeyArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (!compilerTools.contains(kKeyWindowsResource))
	{
		std::string rc;
		rc = Commands::which(toolchain.rc);

		parseArchitecture(rc);
		compilerTools[kKeyWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (!result)
	{
		compilerTools.erase(kKeyCpp);
		compilerTools.erase(kKeyCc);
		compilerTools.erase(kKeyLinker);
		compilerTools.erase(kKeyArchiver);
		compilerTools.erase(kKeyWindowsResource);
	}

	return result;
}

/*****************************************************************************/
bool CacheJsonParser::makeCache()
{
	// TODO: Copy from global cache. If one doesn't exist, do this

	// Create the json cache
	m_jsonFile.makeNode(kKeyWorkingDirectory, JsonDataType::string);
	m_jsonFile.makeNode(kKeySettings, JsonDataType::object);
	m_jsonFile.makeNode(kKeyCompilerTools, JsonDataType::object);
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
		settings[kKeyDumpAssembly] = false;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyMaxJobs) || !settings[kKeyMaxJobs].is_number_integer())
	{
		settings[kKeyMaxJobs] = m_state.environment.processorCount();
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyShowCommands) || !settings[kKeyShowCommands].is_boolean())
	{
		settings[kKeyShowCommands] = false;
		m_jsonFile.setDirty(true);
	}

	if (!settings.contains(kKeyStrategy) || !settings[kKeyStrategy].is_string() || settings[kKeyStrategy].get<std::string>().empty())
	{
		// Note: this is only for validation. it gets changed later
		settings[kKeyStrategy] = "makefile";
		m_jsonFile.setDirty(true);
		m_changeStrategy = true;
	}

	//

	Json& compilerTools = m_jsonFile.json[kKeyCompilerTools];

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

	auto& toolchain = m_inputs.toolchainPreference();
#if defined(CHALET_WIN32)
	if (!makeToolchain(compilerTools, toolchain))
	{
		if (toolchain.type == ToolchainType::MSVC)
		{
			m_inputs.setToolchainPreference("gcc"); // aka mingw
			if (!makeToolchain(compilerTools, toolchain))
			{
				m_inputs.setToolchainPreference("llvm"); // try once more for clang
				makeToolchain(compilerTools, toolchain);
			}
		}
	}
#else
	makeToolchain(compilerTools, toolchain);
#endif

	Json& tools = m_jsonFile.json[kKeyTools];

	whichAdd(tools, kKeyBash);
	whichAdd(tools, kKeyBrew, HostPlatform::MacOS);
	whichAdd(tools, kKeyCmake);
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

	if (!tools.contains(kKeyMake))
	{
#if defined(CHALET_WIN32)
		std::string make;

		// jom.exe - Qt's parallel NMAKE
		// nmake.exe - MSVC's make-ish build tool, alternative to MSBuild
		StringList makeSearches;
		if (toolchain.type != ToolchainType::MSVC)
		{
			makeSearches.push_back("mingw32-make");
		}
		else if (toolchain.type == ToolchainType::MSVC)
		{
			makeSearches.push_back("jom");
			makeSearches.push_back("nmake");
		}
		makeSearches.push_back(kKeyMake);
		for (const auto& tool : makeSearches)
		{
			make = Commands::which(tool);
			if (!make.empty())
			{
				break;
			}
		}
#else
		std::string make = Commands::which(kKeyMake);
#endif

		tools[kKeyMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(tools, kKeyNinja);
	whichAdd(tools, kKeyObjdump);
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
		Diagnostic::error(fmt::format("{}: Json root must be an object.", m_jsonFile.filename()), "Error parsing file");
		return false;
	}

	if (!parseRoot(inJson))
		return false;

	if (!parseSettings(inJson))
		return false;

	if (!parseTools(inJson))
		return false;

	if (!parseCompilers(inJson))
		return false;

#if defined(CHALET_MACOS)
	if (!parseAppleSdks(inJson))
		return false;
#endif
	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseRoot(const Json& inNode)
{
	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyWorkingDirectory))
		m_state.paths.setWorkingDirectory(std::move(val));

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseSettings(const Json& inNode)
{
	if (!inNode.contains(kKeySettings))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeySettings));
		return false;
	}

	const Json& settings = inNode.at(kKeySettings);
	if (!settings.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_jsonFile.filename(), kKeySettings));
		return false;
	}

	if (std::string val; m_jsonFile.assignFromKey(val, settings, kKeyStrategy))
		m_state.environment.setStrategy(val);

	if (bool val = false; m_jsonFile.assignFromKey(val, settings, kKeyShowCommands))
		m_state.environment.setShowCommands(val);

	if (bool val = false; m_jsonFile.assignFromKey(val, inNode, kKeyDumpAssembly))
		m_state.environment.setDumpAssembly(val);

	if (ushort val = 0; m_jsonFile.assignFromKey(val, settings, kKeyMaxJobs))
		m_state.environment.setMaxJobs(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseTools(Json& inNode)
{
	if (!inNode.contains(kKeyTools))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyTools));
		return false;
	}

	Json& tools = inNode.at(kKeyTools);
	if (!tools.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyTools));
		return false;
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyBash))
		m_state.tools.setBash(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyBrew))
		m_state.tools.setBrew(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyCmake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyCmake] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setCmake(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyCodesign))
		m_state.tools.setCodesign(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyCommandPrompt))
		m_state.tools.setCommandPrompt(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyGit))
		m_state.tools.setGit(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyGprof))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyGprof] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setGprof(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyHdiutil))
		m_state.tools.setHdiutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstallNameTool))
		m_state.tools.setInstallNameTool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyInstruments))
		m_state.tools.setInstruments(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLdd))
		m_state.tools.setLdd(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLipo))
		m_state.tools.setLipo(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyLua))
		m_state.tools.setLua(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyMake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyMake] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setMake(std::move(val));
		m_make = std::move(val);
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyNinja))
		m_state.tools.setNinja(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyObjdump))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyObjdump] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setObjdump(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyOsascript))
		m_state.tools.setOsascript(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyOtool))
		m_state.tools.setOtool(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPerl))
		m_state.tools.setPerl(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPlutil))
		m_state.tools.setPlutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPowershell))
		m_state.tools.setPowershell(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPython))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyPython] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setPython(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyPython3))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyPython3] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setPython3(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyRuby))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyRuby] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.tools.setRuby(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeySample))
		m_state.tools.setSample(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeySips))
		m_state.tools.setSips(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyTiffutil))
		m_state.tools.setTiffutil(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcodebuild))
		m_state.tools.setXcodebuild(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcodegen))
		m_state.tools.setXcodegen(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, tools, kKeyXcrun))
		m_state.tools.setXcrun(std::move(val));

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseCompilers(Json& inNode)
{
	if (!inNode.contains(kKeyCompilerTools))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyCompilerTools));
		return false;
	}

	Json& compilerTools = inNode.at(kKeyCompilerTools);
	if (!compilerTools.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_jsonFile.filename(), kKeyCompilerTools));
		return false;
	}

	if (std::string val; m_jsonFile.assignFromKey(val, compilerTools, kKeyArchiver))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyArchiver] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.compilerTools.setArchiver(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, compilerTools, kKeyCpp))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyCpp] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.compilerTools.setCpp(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, compilerTools, kKeyCc))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyCc] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.compilerTools.setCc(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, compilerTools, kKeyLinker))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyLinker] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.compilerTools.setLinker(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, compilerTools, kKeyWindowsResource))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyWindowsResource] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.compilerTools.setRc(std::move(val));
	}

	return true;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
bool CacheJsonParser::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(kKeyApplePlatformSdks))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_jsonFile.filename(), kKeyApplePlatformSdks));
		return false;
	}

	Json& platformSdks = inNode.at(kKeyApplePlatformSdks);
	for (auto& [key, pathJson] : platformSdks.items())
	{
		if (!pathJson.is_string())
		{
			Diagnostic::error(fmt::format("{}: apple platform '{}' must be a string.", m_jsonFile.filename(), key));
			return false;
		}

		auto path = pathJson.get<std::string>();
		m_state.tools.addApplePlatformSdk(key, std::move(path));
	}

	return true;
}
#endif

/*****************************************************************************/
bool CacheJsonParser::parseArchitecture(std::string& outString) const
{
	bool ret = true;
#if defined(CHALET_WIN32)

	if (String::contains({ "/mingw64/", "/mingw32/" }, outString))
	{
		std::string lower = String::toLowerCase(outString);
		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
		{
			auto start = lower.find("/mingw32/");
			if (start != std::string::npos)
			{
				std::string tmp = outString;
				String::replaceAll(tmp, tmp.substr(start, 9), "/mingw64/");
				if (Commands::pathExists(tmp))
				{
					outString = tmp;
				}
				ret = false;
			}
		}
		else if (m_state.info.targetArchitecture() == Arch::Cpu::X86)
		{
			auto start = lower.find("/mingw64/");
			if (start != std::string::npos)
			{
				std::string tmp = outString;
				String::replaceAll(tmp, tmp.substr(start, 9), "/mingw32/");
				if (Commands::pathExists(tmp))
				{
					outString = tmp;
				}
				ret = false;
			}
		}
	}
	else if (String::contains({ "/clang64/", "/clang32/" }, outString))
	{
		// TODO: clangarm64

		std::string lower = String::toLowerCase(outString);
		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
		{
			auto start = lower.find("/clang32/");
			if (start != std::string::npos)
			{
				std::string tmp = outString;
				String::replaceAll(tmp, tmp.substr(start, 9), "/clang64/");
				if (Commands::pathExists(tmp))
				{
					outString = tmp;
				}
				ret = false;
			}
		}
		else if (m_state.info.targetArchitecture() == Arch::Cpu::X86)
		{
			auto start = lower.find("/clang64/");
			if (start != std::string::npos)
			{
				std::string tmp = outString;
				String::replaceAll(tmp, tmp.substr(start, 9), "/clang32/");
				if (Commands::pathExists(tmp))
				{
					outString = tmp;
				}
				ret = false;
			}
		}
	}
	else if (String::endsWith({ "cl.exe", "link.exe", "lib.exe" }, outString))
	{
		std::string lower = String::toLowerCase(outString);
		if (m_state.info.hostArchitecture() == Arch::Cpu::X64)
		{
			auto start = lower.find("/hostx86/");
			if (start != std::string::npos)
			{
				String::replaceAll(outString, outString.substr(start, 9), "/HostX64/");
				ret = false;
			}
		}
		else if (m_state.info.hostArchitecture() == Arch::Cpu::X86)
		{
			auto start = lower.find("/hostx64/");
			if (start != std::string::npos)
			{
				String::replaceAll(outString, outString.substr(start, 9), "/HostX86/");
				ret = false;
			}
		}

		if (m_state.info.targetArchitecture() == Arch::Cpu::X64)
		{
			auto start = lower.find("/x86/");
			if (start != std::string::npos)
			{
				String::replaceAll(outString, outString.substr(start, 5), "/x64/");
				ret = false;
			}
		}
		else if (m_state.info.targetArchitecture() == Arch::Cpu::X86)
		{
			auto start = lower.find("/x64/");
			if (start != std::string::npos)
			{
				String::replaceAll(outString, outString.substr(start, 5), "/x86/");
				ret = false;
			}
		}
	}

#else
	UNUSED(outString);
#endif

	return ret;
}
}
