/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonParser.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

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
	auto& environmentCache = m_state.cache.environmentCache();
	Json cacheJsonSchema = Schema::getCacheJson();

	if (m_inputs.saveSchemaToFile())
	{
		JsonFile::saveToFile(cacheJsonSchema, "schema/chalet-cache.schema.json");
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

	if (!validatePaths())
		return false;

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::validatePaths()
{
	auto& compilers = m_state.compilers;
#if defined(CHALET_DEBUG)
	auto& cacheJson = m_state.cache.environmentCache();
#endif

	if (!Commands::pathExists(compilers.cpp()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: C++ compiler could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilers.cc()))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: C compiler could not be found.", m_filename));
		return false;
	}

#if defined(CHALET_WIN32)
	if (!Commands::pathExists(compilers.rc()))
	{
	#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
	#endif
		Diagnostic::warn(fmt::format("{}: Windows Resource compiler could not be found.", m_filename));
	}
#endif

	if (!Commands::pathExists(m_make))
	{
#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
#endif
		Diagnostic::errorAbort(fmt::format("{}: 'make' could not be found.", m_filename));
		return false;
	}

#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_state.tools.macosSdk()))
	{
	#if defined(CHALET_DEBUG)
		cacheJson.dumpToTerminal();
	#endif
		Diagnostic::errorAbort(fmt::format("{}: 'No MacOS SDK path could be found. Please install either Xcode or Command Line Tools.", m_filename));
	}
#endif

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
	environmentCache.makeNode(kKeyCompilers, JsonDataType::object);
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
			if (Environment::isContinuousIntegrationServer())
				strategyJson = "native-experimental";
			else if (Environment::isMsvc())
				strategyJson = "ninja";
			else
				strategyJson = "makefile";
		}
	}

	Json& compilers = environmentCache.json[kKeyCompilers];

	auto whichAdd = [&](Json& inNode, const std::string& inKey) -> bool {
		if (!inNode.contains(inKey))
		{
			auto path = Commands::which(inKey);
			bool res = !path.empty();
			inNode[inKey] = std::move(path);
			m_state.cache.setDirty(true);
			return res;
		}

		return true;
	};

	if (!compilers.contains(kKeyCpp))
	{
		auto varCXX = Environment::get("CXX");
		std::string cpp;
		if (varCXX != nullptr)
			cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
#if defined(CHALET_WIN32)
			for (const auto& compiler : { "cl", "clang++", "g++", "c++" })
#else
			for (const auto& compiler : { "clang++", "g++", "c++" })
#endif
			{
				cpp = Commands::which(compiler);
				if (!cpp.empty())
				{
					break;
				}
			}
		}

		compilers[kKeyCpp] = std::move(cpp);
		m_state.cache.setDirty(true);
	}

	if (!compilers.contains(kKeyCc))
	{
		auto varCC = Environment::get("CC");
		std::string cc;
		if (varCC != nullptr)
			cc = Commands::which(varCC);

		if (cc.empty())
		{
#if defined(CHALET_WIN32)
			for (const auto& compiler : { "cl", "clang", "gcc", "cc" })
#else
			for (const auto& compiler : { "clang", "gcc", "cc" })
#endif
			{
				cc = Commands::which(compiler);
				if (!cc.empty())
				{
					break;
				}
			}
		}

		compilers[kKeyCc] = std::move(cc);
		m_state.cache.setDirty(true);
	}

	if (!compilers.contains(kKeyWindowsResource))
	{
		auto rc = Commands::which("windres");
#if defined(CHALET_WIN32)
		if (rc.empty())
			rc = Commands::which("rc");
#endif

		compilers[kKeyWindowsResource] = std::move(rc);
		m_state.cache.setDirty(true);
	}

	Json& tools = environmentCache.json[kKeyTools];

	whichAdd(tools, kKeyAr);
	whichAdd(tools, kKeyBash);
	whichAdd(tools, kKeyBrew);
	whichAdd(tools, kKeyCmake);
	whichAdd(tools, kKeyCodesign);

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
	whichAdd(tools, kKeyHdiutil);
	whichAdd(tools, kKeyInstallNameTool);
	whichAdd(tools, kKeyInstruments);
	whichAdd(tools, kKeyLdd);
	whichAdd(tools, kKeyLibTool);
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
		for (const auto& tool : { "nmake", "mingw32-make", "make" })
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
	whichAdd(tools, kKeyOsascript);
	whichAdd(tools, kKeyOtool);
	whichAdd(tools, kKeyPerl);
	whichAdd(tools, kKeyPlutil);
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

	whichAdd(tools, kKeyRanlib);
	whichAdd(tools, kKeyRuby);
	whichAdd(tools, kKeySample);
	whichAdd(tools, kKeySips);
	whichAdd(tools, kKeyStrip);
	whichAdd(tools, kKeyTiffutil);
	whichAdd(tools, kKeyXcodebuild);
	whichAdd(tools, kKeyXcodegen);
	whichAdd(tools, kKeyXcrun);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::serializeFromJsonRoot(const Json& inJson)
{
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
	if (!inNode.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: Json root must be an object.", m_filename), "Error parsing file");
		return false;
	}

	if (std::string val; JsonNode::assignFromKey(val, inNode, kKeyStrategy))
		m_state.environment.setStrategy(val);

	if (std::string val; JsonNode::assignFromKey(val, inNode, kKeyWorkingDirectory))
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

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyAr))
		m_state.tools.setAr(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyBash))
		m_state.tools.setBash(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyBrew))
		m_state.tools.setBrew(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyCmake))
		m_state.tools.setCmake(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyCodesign))
		m_state.tools.setCodesign(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyCommandPrompt))
		m_state.tools.setCommandPrompt(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyGit))
		m_state.tools.setGit(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyGprof))
		m_state.tools.setGprof(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyHdiutil))
		m_state.tools.setHdiutil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyInstallNameTool))
		m_state.tools.setInstallNameUtil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyInstruments))
		m_state.tools.setInstruments(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyMacosSdk))
		m_state.tools.setMacosSdk(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyLdd))
		m_state.tools.setLdd(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyLibTool))
		m_state.tools.setLibtool(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyLua))
		m_state.tools.setLua(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyMake))
	{
		m_state.tools.setMake(val);
		m_make = std::move(val);
	}

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyNinja))
		m_state.tools.setNinja(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyObjdump))
		m_state.tools.setObjdump(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyOsascript))
		m_state.tools.setOsascript(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyOtool))
		m_state.tools.setOtool(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPerl))
		m_state.tools.setPerl(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPlutil))
		m_state.tools.setPlutil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPowershell))
		m_state.tools.setPowershell(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPython))
		m_state.tools.setPython(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPython3))
		m_state.tools.setPython3(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyRanlib))
		m_state.tools.setRanlib(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyRuby))
		m_state.tools.setRuby(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeySample))
		m_state.tools.setSample(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeySips))
		m_state.tools.setSips(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyStrip))
		m_state.tools.setStrip(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyTiffutil))
		m_state.tools.setTiffutil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyXcodebuild))
		m_state.tools.setXcodebuild(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyXcodegen))
		m_state.tools.setXcodegen(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyXcrun))
		m_state.tools.setXcrun(val);

	return true;
}

/*****************************************************************************/
bool CacheJsonParser::parseCompilers(const Json& inNode)
{
	if (!inNode.contains(kKeyCompilers))
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' is required, but was not found.", m_filename, kKeyCompilers));
		return false;
	}

	const Json& compilers = inNode.at(kKeyCompilers);
	if (!compilers.is_object())
	{
		Diagnostic::errorAbort(fmt::format("{}: '{}' must be an object.", m_filename, kKeyCompilers));
		return false;
	}

	if (std::string val; JsonNode::assignFromKey(val, compilers, kKeyCpp))
		m_state.compilers.setCpp(val);

	if (std::string val; JsonNode::assignFromKey(val, compilers, kKeyCc))
		m_state.compilers.setCc(val);

	if (std::string val; JsonNode::assignFromKey(val, compilers, kKeyWindowsResource))
		m_state.compilers.setRc(val);

	return true;
}

}
