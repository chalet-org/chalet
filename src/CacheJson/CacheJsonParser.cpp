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

	const auto& jRoot = environmentCache.json;
	if (!serializeFromJsonRoot(jRoot))
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
		Diagnostic::errorAbort(fmt::format("{}: The toolchain's C++ compiler was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.cc()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: The toolchain's C compiler was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.archiver()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: The toolchain's archive utility was blank or could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilerTools.linker()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: The toolchain's linker was blank or could not be found.", m_filename));
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
		Diagnostic::errorAbort(fmt::format("{}: 'make' could not be found.", m_filename));
		return false;
	}
	*/

#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_state.tools.macosSdk()))
	{
	#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
	#endif
		Diagnostic::errorAbort(fmt::format("{}: 'No MacOS SDK path could be found. Please install either Xcode or Command Line Tools.", m_filename));
		return false;
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
	Json& strategyJson = environmentCache.json[kKeyStrategy];

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

	environmentCache.makeNode(kKeyStrategy, JsonDataType::string);
	environmentCache.makeNode(kKeyWorkingDirectory, JsonDataType::string);
	environmentCache.makeNode(kKeyCompilerTools, JsonDataType::object);
	environmentCache.makeNode(kKeyTools, JsonDataType::object);
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

	{
		Json& strategyJson = environmentCache.json[kKeyStrategy];
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

	if (!tools.contains(kKeyMacosSdk))
	{
#if defined(CHALET_MACOS)
		std::string sdkPath = Commands::subprocessOutput({ "xcrun", "--sdk", "macosx", "--show-sdk-path" });
		tools[kKeyMacosSdk] = std::move(sdkPath);
#else
		tools[kKeyMacosSdk] = std::string();
#endif
		m_state.cache.setDirty(true);
	}

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

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::serializeFromJsonRoot(const Json& inJson)
{
	if (!inJson.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: Json root must be an object.", m_filename), "Error parsing file");
		return false;
	}

	if (!parseRoot(inJson))
		return false;

	if (!parseTools(inJson))
		return false;

	if (!parseCompilers(inJson))
		return false;

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseRoot(const Json& inNode)
{
	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, inNode, kKeyStrategy))
		m_state.environment.setStrategy(val);

	if (std::string val; environmentCache.assignFromKey(val, inNode, kKeyWorkingDirectory))
		m_state.paths.setWorkingDirectory(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseTools(const Json& inNode)
{
	if (!inNode.contains(kKeyTools))
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyTools));
		return false;
	}

	const Json& tools = inNode.at(kKeyTools);
	if (!tools.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' must be an object.", m_filename, kKeyTools));
		return false;
	}

	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyBash))
		m_state.tools.setBash(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyBrew))
		m_state.tools.setBrew(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCmake))
		m_state.tools.setCmake(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCodesign))
		m_state.tools.setCodesign(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyCommandPrompt))
		m_state.tools.setCommandPrompt(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyGit))
		m_state.tools.setGit(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyGprof))
		m_state.tools.setGprof(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyHdiutil))
		m_state.tools.setHdiutil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyInstallNameTool))
		m_state.tools.setInstallNameUtil(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyInstruments))
		m_state.tools.setInstruments(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyMacosSdk))
		m_state.tools.setMacosSdk(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyLdd))
		m_state.tools.setLdd(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyLua))
		m_state.tools.setLua(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyMake))
	{
		m_state.tools.setMake(val);
		m_make = std::move(val);
	}

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyNinja))
		m_state.tools.setNinja(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyObjdump))
		m_state.tools.setObjdump(val);

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
		m_state.tools.setPython(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyPython3))
		m_state.tools.setPython3(val);

	if (std::string val; environmentCache.assignFromKey(val, tools, kKeyRuby))
		m_state.tools.setRuby(val);

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
bool CacheJsonParser::parseCompilers(const Json& inNode)
{
	if (!inNode.contains(kKeyCompilerTools))
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyCompilerTools));
		return false;
	}

	const Json& compilerTools = inNode.at(kKeyCompilerTools);
	if (!compilerTools.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' must be an object.", m_filename, kKeyCompilerTools));
		return false;
	}

	auto& environmentCache = m_state.cache.environmentCache();
	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyArchiver))
		m_state.compilerTools.setArchiver(val);

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyCpp))
		m_state.compilerTools.setCpp(val);

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyCc))
		m_state.compilerTools.setCc(val);

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyLinker))
		m_state.compilerTools.setLinker(val);

	if (std::string val; environmentCache.assignFromKey(val, compilerTools, kKeyWindowsResource))
		m_state.compilerTools.setRc(val);

	return true;
}
}
