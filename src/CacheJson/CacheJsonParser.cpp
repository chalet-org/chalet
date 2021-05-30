/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonParser.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Core/HostPlatform.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
CacheJsonParser::CacheJsonParser(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState),
	m_filename(m_state.cache.environmentCache().filename())
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

	auto& environmentCache = m_state.cache.environmentCache();
	Json cacheJsonSchema = Schema::getCacheJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(cacheJsonSchema, "schema/chalet-cache.schema.json");
	}

	Timer timer;
	bool cacheExists = m_state.cache.exists();
	if (cacheExists)
	{
		Diagnostic::info(fmt::format("Reading Cache [{}]", m_state.cache.environmentCache().filename()), false);
	}
	else
	{
		Diagnostic::info(fmt::format("Creating Cache [{}]", m_state.cache.environmentCache().filename()), false);
	}

	if (!makeCache())
		return false;

	if (!environmentCache.validate(std::move(cacheJsonSchema)))
		return false;

	if (!serializeFromJsonRoot(environmentCache.json))
	{
		Diagnostic::error(fmt::format("There was an error parsing {}", m_filename));
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

	auto& environmentCache = m_state.cache.environmentCache();
	const auto& jRoot = environmentCache.json;
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

				bool result = false;
				result |= keyEndsWith(kKeyCpp, "cl.exe");
				result |= keyEndsWith(kKeyCc, "cl.exe");
				result |= keyEndsWith(kKeyLinker, "link.exe");
				result |= keyEndsWith(kKeyArchiver, "lib.exe");
				result |= keyEndsWith(kKeyWindowsResource, "rc.exe");
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
#if defined(CHALET_DEBUG)
	auto& cacheJson = m_state.cache.environmentCache();
#endif

	if (!Commands::pathExists(compilerTools.cpp()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C++ compiler was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.cc()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C compiler was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.archiver()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's archive utility was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.linker()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's linker was blank or could not be found.", m_filename));
		return false;
	}

#if defined(CHALET_WIN32)
	if (!Commands::pathExists(compilerTools.rc()))
	{
	#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
	#endif
		Diagnostic::warn(fmt::format("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_filename));
	}
#endif

	UNUSED(m_make);
	/*
	if (!Commands::pathExists(m_make))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: 'make' could not be found.", m_filename));
		return false;
	}
	*/

#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_state.tools.applePlatformSdk("macosx")))
	{
	#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
	#endif
		Diagnostic::error(fmt::format("{}: 'No MacOS SDK path could be found. Please install either Xcode or Command Line Tools.", m_filename));
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
				m_state.info.setTargetArchitecture("x86_64-pc-win32");
				break;

			case Arch::Cpu::X86:
				m_state.info.setTargetArchitecture("i686-pc-win32");
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
	bool usingMsvc = false;
	{
		usingMsvc |= String::endsWith("cl.exe", m_state.compilerTools.cc());
		usingMsvc |= String::endsWith("cl.exe", m_state.compilerTools.cpp());
		usingMsvc |= String::endsWith("link.exe", m_state.compilerTools.linker());
		usingMsvc |= String::endsWith("lib.exe", m_state.compilerTools.archiver());
		usingMsvc |= String::endsWith("rc.exe", m_state.compilerTools.rc());

		if (!usingMsvc)
		{
			m_state.msvcEnvironment.cleanup();
		}
	}

	auto& environmentCache = m_state.cache.environmentCache();
	Json& settings = environmentCache.json[kKeySettings];
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

		m_state.cache.setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::makeCache()
{
	// TODO: Copy from global cache. If one doesn't exist, do this

	// Create the json cache
	auto& environmentCache = m_state.cache.environmentCache();

	environmentCache.makeNode(kKeyWorkingDirectory, JsonDataType::string);
	environmentCache.makeNode(kKeySettings, JsonDataType::object);
	environmentCache.makeNode(kKeyCompilerTools, JsonDataType::object);
	environmentCache.makeNode(kKeyTools, JsonDataType::object);
	environmentCache.makeNode(kKeyApplePlatformSdks, JsonDataType::object);
	environmentCache.makeNode(kKeyExternalDependencies, JsonDataType::object);
	environmentCache.makeNode(kKeyData, JsonDataType::object);

	{
		Json& workingDirectoryJson = environmentCache.json[kKeyWorkingDirectory];
		const auto workingDirectory = workingDirectoryJson.get<std::string>();

		if (workingDirectory.empty())
		{
			auto tmp = Commands::getWorkingDirectory();
			Path::sanitize(tmp, true);
			workingDirectoryJson = std::move(tmp);
		}
	}

	Json& settings = environmentCache.json[kKeySettings];
	if (!settings.contains(kKeyStrategy))
		settings[kKeyStrategy] = std::string();

	{
		Json& strategyJson = settings[kKeyStrategy];

		const auto strategy = strategyJson.get<std::string>();

		if (strategy.empty())
		{
			// Note: this is only for validation. it gets changed later
			strategyJson = "makefile";
			m_changeStrategy = true;
		}
	}

	//

	Json& compilerTools = environmentCache.json[kKeyCompilerTools];

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
				m_state.cache.setDirty(true);
				return res;
			}
			else
			{
				inNode[inKey] = std::string();
				m_state.cache.setDirty(true);
				return true;
			}
		}

		return true;
	};

	if (!compilerTools.contains(kKeyCpp))
	{
		auto varCXX = Environment::get("CXX");
		std::string cpp;
		if (varCXX != nullptr)
			cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
#if defined(CHALET_WIN32)
			for (const auto& compiler : { "clang++", "g++", "c++", "cl" })
#else
			for (const auto& compiler : { "clang++", "g++", "c++" })
#endif
			{
				cpp = Commands::which(compiler);
				if (!cpp.empty())
					break;
			}
		}

		parseArchitecture(cpp);

		compilerTools[kKeyCpp] = std::move(cpp);
		m_state.cache.setDirty(true);
	}

	if (!compilerTools.contains(kKeyCc))
	{
		auto varCC = Environment::get("CC");
		std::string cc;
		if (varCC != nullptr)
			cc = Commands::which(varCC);

		if (cc.empty())
		{
#if defined(CHALET_WIN32)
			for (const auto& compiler : { "clang", "gcc", "cc", "cl" })
#else
			for (const auto& compiler : { "clang", "gcc", "cc" })
#endif
			{
				cc = Commands::which(compiler);
				if (!cc.empty())
					break;
			}
		}

		parseArchitecture(cc);
		compilerTools[kKeyCc] = std::move(cc);
		m_state.cache.setDirty(true);
	}

	if (!compilerTools.contains(kKeyLinker))
	{
		std::string link;
#if defined(CHALET_WIN32)
		for (const auto& linker : { "lld", "ld", "link" })
#else
		for (const auto& linker : { "lld", "ld" })
#endif
		{
			link = Commands::which(linker);
			if (!link.empty())
				break;
		}

		parseArchitecture(link);
		compilerTools[kKeyLinker] = std::move(link);
		m_state.cache.setDirty(true);
	}

	if (!compilerTools.contains(kKeyArchiver))
	{
		std::string ar;
#if defined(CHALET_WIN32)
		for (const auto& archiver : { "ar", "lib" })
#else
		for (const auto& archiver : { "libtool", "ar" })
#endif
		{
			ar = Commands::which(archiver);
			if (!ar.empty())
				break;
		}

		parseArchitecture(ar);
		compilerTools[kKeyArchiver] = std::move(ar);
		m_state.cache.setDirty(true);
	}

	if (!compilerTools.contains(kKeyWindowsResource))
	{
		auto rc = Commands::which("windres");
#if defined(CHALET_WIN32)
		if (rc.empty())
			rc = Commands::which("rc");
#endif

		parseArchitecture(rc);
		compilerTools[kKeyWindowsResource] = std::move(rc);
		m_state.cache.setDirty(true);
	}

	//

	Json& tools = environmentCache.json[kKeyTools];

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
		m_state.cache.setDirty(true);
	}

	whichAdd(tools, kKeyGit);
	whichAdd(tools, kKeyGprof);
	whichAdd(tools, kKeyHdiutil, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstallNameTool, HostPlatform::MacOS);
	whichAdd(tools, kKeyInstruments, HostPlatform::MacOS);
	whichAdd(tools, kKeyLdd);
	whichAdd(tools, kKeyLua);

	if (!tools.contains(kKeyMake))
	{
#if defined(CHALET_WIN32)
		std::string make;

		// jom.exe - Qt's parallel NMAKE
		// nmake.exe - MSVC's make-ish build tool, alternative to MSBuild
		StringList makeSearches{ "mingw32-make", "jom", "nmake" };
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
		m_state.cache.setDirty(true);
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
		m_state.cache.setDirty(true);
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
	Json& platformSdksJson = environmentCache.json[kKeyApplePlatformSdks];

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
			m_state.cache.setDirty(true);
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
		Diagnostic::error(fmt::format("{}: Json root must be an object.", m_filename), "Error parsing file");
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
	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, inNode, kKeyWorkingDirectory))
		m_state.paths.setWorkingDirectory(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseSettings(const Json& inNode)
{
	if (!inNode.contains(kKeySettings))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeySettings));
		return false;
	}

	const Json& settings = inNode.at(kKeySettings);
	if (!settings.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_filename, kKeySettings));
		return false;
	}

	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, settings, kKeyStrategy))
		m_state.environment.setStrategy(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseTools(Json& inNode)
{
	if (!inNode.contains(kKeyTools))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyTools));
		return false;
	}

	Json& tools = inNode.at(kKeyTools);
	if (!tools.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_filename, kKeyTools));
		return false;
	}

	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyBash))
		m_state.tools.setBash(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyBrew))
		m_state.tools.setBrew(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCmake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyCmake] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setCmake(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCodesign))
		m_state.tools.setCodesign(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCommandPrompt))
		m_state.tools.setCommandPrompt(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyGit))
		m_state.tools.setGit(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyGprof))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyGprof] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setGprof(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyHdiutil))
		m_state.tools.setHdiutil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyInstallNameTool))
		m_state.tools.setInstallNameUtil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyInstruments))
		m_state.tools.setInstruments(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyLdd))
		m_state.tools.setLdd(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyLua))
		m_state.tools.setLua(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyMake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyMake] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setMake(val);
		m_make = std::move(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyNinja))
		m_state.tools.setNinja(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyObjdump))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyObjdump] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setObjdump(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyOsascript))
		m_state.tools.setOsascript(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyOtool))
		m_state.tools.setOtool(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPerl))
		m_state.tools.setPerl(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPlutil))
		m_state.tools.setPlutil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPowershell))
		m_state.tools.setPowershell(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPython))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyPython] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setPython(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPython3))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyPython3] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setPython3(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyRuby))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			tools[kKeyRuby] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.tools.setRuby(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeySample))
		m_state.tools.setSample(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeySips))
		m_state.tools.setSips(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyTiffutil))
		m_state.tools.setTiffutil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyXcodebuild))
		m_state.tools.setXcodebuild(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyXcodegen))
		m_state.tools.setXcodegen(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyXcrun))
		m_state.tools.setXcrun(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseCompilers(Json& inNode)
{
	if (!inNode.contains(kKeyCompilerTools))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyCompilerTools));
		return false;
	}

	Json& compilerTools = inNode.at(kKeyCompilerTools);
	if (!compilerTools.is_object())
	{
		Diagnostic::error(fmt::format("{}: '{}' must be an object.", m_filename, kKeyCompilerTools));
		return false;
	}

	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyArchiver))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyArchiver] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.compilerTools.setArchiver(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyCpp))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyCpp] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.compilerTools.setCpp(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyCc))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyCc] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.compilerTools.setCc(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyLinker))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyLinker] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.compilerTools.setLinker(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyWindowsResource))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			compilerTools[kKeyWindowsResource] = val;
			m_state.cache.setDirty(true);
		}
#endif
		m_state.compilerTools.setRc(val);
	}

	return true;
}

#if defined(CHALET_MACOS)
bool CacheJsonParser::parseAppleSdks(Json& inNode)
{
	if (!inNode.contains(kKeyApplePlatformSdks))
	{
		Diagnostic::error(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyApplePlatformSdks));
		return false;
	}

	Json& platformSdks = inNode.at(kKeyApplePlatformSdks);
	for (auto& [key, pathJson] : platformSdks.items())
	{
		if (!pathJson.is_string())
		{
			Diagnostic::error(fmt::format("{}: apple platform '{}' must be a string.", m_filename, key));
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
