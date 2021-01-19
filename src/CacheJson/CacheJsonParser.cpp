/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheJsonParser.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonComments.hpp"

namespace chalet
{
/*****************************************************************************/
CacheJsonParser::CacheJsonParser(BuildState& inState) :
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
#ifdef CHALET_DEBUG
	JsonFile::saveToFile(cacheJsonSchema, "schema/chalet-cache.schema.json");
#endif

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

	if (!Commands::pathExists(compilers.cpp()))
	{
		Diagnostic::errorAbort(fmt::format("{}: C++ compiler could not be found.", m_filename));
		return false;
	}

	if (!Commands::pathExists(compilers.cc()))
	{
		Diagnostic::errorAbort(fmt::format("{}: C compiler could not be found.", m_filename));
		return false;
	}

#if defined(CHALET_WIN32)
	if (!Commands::pathExists(compilers.rc()))
	{
		Diagnostic::warn(fmt::format("{}: Windows Resource compiler could not be found.", m_filename));
	}
#endif

	if (!Commands::pathExists(m_make))
	{
		Diagnostic::errorAbort(fmt::format("{}: 'make' could not be found.", m_filename));
		return false;
	}

#if defined(CHALET_MACOS)
	if (!Commands::pathExists(m_state.tools.macosSdk()))
	{
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
			strategyJson = "makefile";
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
		auto cpp = Commands::which("clang++");
		if (cpp.empty())
		{
			cpp = Commands::which("g++");
			if (cpp.empty())
			{
				cpp = Commands::which("c++");
			}
		}

		compilers[kKeyCpp] = std::move(cpp);
		m_state.cache.setDirty(true);
	}

	if (!compilers.contains(kKeyCc))
	{
		auto cc = Commands::which("clang");
		if (cc.empty())
		{
			cc = Commands::which("gcc");
			if (cc.empty())
			{
				cc = Commands::which("cc");
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
	whichAdd(tools, kKeyBrew);
	whichAdd(tools, kKeyCmake);
	whichAdd(tools, kKeyCodeSign);
	whichAdd(tools, kKeyGit);
	whichAdd(tools, kKeyHdiUtil);
	whichAdd(tools, kKeyInstallNameTool);
	whichAdd(tools, kKeyLdd);

	if (!tools.contains(kKeyMacosSdk))
	{
#if defined(CHALET_MACOS)
		std::string sdkPath = Commands::shellWithOutput("xcrun --sdk macosx --show-sdk-path");
		String::replaceAll(sdkPath, "\n", "");
		tools[kKeyMacosSdk] = std::move(sdkPath);
#else
		tools[kKeyMacosSdk] = std::string();
#endif
		m_state.cache.setDirty(true);
	}

	if (!tools.contains(kKeyMake))
	{
		std::string make = Commands::which(kKeyMake);
#if defined(CHALET_WIN32)
		if (make.empty())
			make = Commands::which("mingw32-make");
#endif

		tools[kKeyMake] = std::move(make);
		m_state.cache.setDirty(true);
	}

	whichAdd(tools, kKeyMakeIcns);
	whichAdd(tools, kKeyNinja);
	whichAdd(tools, kKeyOtool);
	whichAdd(tools, kKeyPlUtil);
	whichAdd(tools, kKeyRanLib);
	whichAdd(tools, kKeyStrip);
	whichAdd(tools, kKeyTiffUtil);

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

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyBrew))
		m_state.tools.setBrew(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyCmake))
		m_state.tools.setCmake(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyCodeSign))
		m_state.tools.setCodeSign(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyGit))
		m_state.tools.setGit(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyHdiUtil))
		m_state.tools.setHdiUtil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyInstallNameTool))
		m_state.tools.setInstallNameUtil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyMacosSdk))
		m_state.tools.setMacosSdk(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyLdd))
		m_state.tools.setLdd(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyMake))
	{
		m_state.tools.setMake(val);
		m_make = std::move(val);
	}

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyMakeIcns))
		m_state.tools.setMakeIcns(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyNinja))
		m_state.tools.setNinja(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyOtool))
		m_state.tools.setOtool(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyPlUtil))
		m_state.tools.setPlUtil(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyRanLib))
		m_state.tools.setRanlib(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyStrip))
		m_state.tools.setStrip(val);

	if (std::string val; JsonNode::assignFromKey(val, tools, kKeyTiffUtil))
		m_state.tools.setTiffUtil(val);

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
