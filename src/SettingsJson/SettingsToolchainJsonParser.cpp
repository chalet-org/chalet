/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsToolchainJsonParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsToolchainJsonParser::SettingsToolchainJsonParser(const CommandLineInputs& inInputs, BuildState& inState, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_state(inState),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::serialize()
{
	Output::setShowCommandOverride(false);

	auto& rootNode = m_jsonFile.json;
	if (std::string val; m_jsonFile.assignFromKey(val, rootNode, kKeyWorkingDirectory))
		m_state.paths.setWorkingDirectory(std::move(val));

	const auto& preference = m_inputs.toolchainPreferenceRaw();
	auto& toolchains = rootNode["toolchains"];
	if (!toolchains.contains(preference))
	{
		toolchains[preference] = JsonDataType::object;
	}

	auto& node = toolchains.at(preference);
	if (!serialize(node))
		return false;

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::serialize(Json& inNode)
{
	if (!inNode.is_object())
		return false;

	auto& toolchain = m_inputs.toolchainPreference();
#if defined(CHALET_WIN32)
	if (toolchain.type == ToolchainType::MSVC)
	{
		if (!m_state.msvcEnvironment.create())
			return false;
	}
#endif

	makeToolchain(inNode, toolchain);

	if (!parseToolchain(inNode))
		return false;

	if (!validatePaths())
		return false;

	return true;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::validatePaths()
{
	bool result = true;
	if (m_state.toolchain.cpp().empty() || !Commands::pathExists(m_state.toolchain.cpp()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (m_state.toolchain.cc().empty() || !Commands::pathExists(m_state.toolchain.cc()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's C compiler was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (m_state.toolchain.archiver().empty() || !Commands::pathExists(m_state.toolchain.archiver()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's archive utility was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (m_state.toolchain.linker().empty() || !Commands::pathExists(m_state.toolchain.linker()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's linker was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

#if defined(CHALET_WIN32)
	if (m_state.toolchain.rc().empty() || !Commands::pathExists(m_state.toolchain.rc()))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::warn("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename());
	}
#endif
	if (!result)
	{
		auto& preference = m_inputs.toolchainPreferenceRaw();
		Diagnostic::error("{}: The requested toolchain of '{}' could either not be detected, or had invalid tools.", m_jsonFile.filename(), preference);
	}

	return result;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::makeToolchain(Json& toolchains, const ToolchainPreference& toolchain)
{
	bool result = true;

	if (toolchain.type == ToolchainType::Unknown)
		return result;

	if (!toolchains.contains(kKeyStrategy) || !toolchains[kKeyStrategy].is_string() || toolchains[kKeyStrategy].get<std::string>().empty())
	{
		toolchains[kKeyStrategy] = std::string();
	}

	std::string cpp;
	std::string cc;
	if (!toolchains.contains(kKeyCpp))
	{
		// auto varCXX = Environment::get("CXX");
		// if (varCXX != nullptr)
		// 	cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
			cpp = Commands::which(toolchain.cpp);
		}

		result &= !cpp.empty();

		toolchains[kKeyCpp] = cpp;
		m_jsonFile.setDirty(true);
	}
	if (!toolchains.contains(kKeyCc) || !toolchains[kKeyCc].is_string() || toolchains[kKeyCc].get<std::string>().empty())
	{
		// auto varCC = Environment::get("CC");
		// if (varCC != nullptr)
		// 	cc = Commands::which(varCC);

		if (cc.empty())
		{
			cc = Commands::which(toolchain.cc);
		}

		result &= !cc.empty();

		toolchains[kKeyCc] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyLinker) || !toolchains[kKeyLinker].is_string() || toolchains[kKeyLinker].get<std::string>().empty())
	{
		std::string link;
		StringList searches;
		searches.push_back(toolchain.linker);
		if (toolchain.type == ToolchainType::LLVM)
		{
			searches.emplace_back("ld");
		}

		for (const auto& search : searches)
		{
			link = Commands::which(search);
			if (!link.empty())
				break;
		}

#if defined(CHALET_WIN32)
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
#endif

		result &= !link.empty();

		toolchains[kKeyLinker] = std::move(link);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyArchiver) || !toolchains[kKeyArchiver].is_string() || toolchains[kKeyArchiver].get<std::string>().empty())
	{
		std::string ar;
		StringList searches;
		if (toolchain.type == ToolchainType::LLVM)
		{
			searches.emplace_back("llvm-ar");
		}
		if (toolchain.type == ToolchainType::LLVM || toolchain.type == ToolchainType::GNU)
		{
			searches.emplace_back("libtool");
		}
		searches.push_back(toolchain.archiver);

		for (const auto& search : searches)
		{
			ar = Commands::which(search);
			if (!ar.empty())
				break;
		}

		result &= !ar.empty();

		toolchains[kKeyArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyProfiler) || !toolchains[kKeyProfiler].is_string() || toolchains[kKeyProfiler].get<std::string>().empty())
	{
		std::string prof;
		StringList searches;
		searches.push_back(toolchain.profiler);

		for (const auto& search : searches)
		{
			prof = Commands::which(search);
			if (!prof.empty())
				break;
		}

		result &= !prof.empty();

		toolchains[kKeyProfiler] = std::move(prof);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyWindowsResource) || !toolchains[kKeyWindowsResource].is_string() || toolchains[kKeyWindowsResource].get<std::string>().empty())
	{
		std::string rc;
		StringList searches;
		searches.push_back(toolchain.rc);

		for (const auto& search : searches)
		{
			rc = Commands::which(search);
			if (!rc.empty())
				break;
		}

		toolchains[kKeyWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	// if (!result)
	// {
	// 	toolchains.erase(kKeyCpp);
	// 	toolchains.erase(kKeyCc);
	// 	toolchains.erase(kKeyLinker);
	// 	toolchains.erase(kKeyArchiver);
	// 	toolchains.erase(kKeyWindowsResource);
	// }

	auto whichAdd = [&](Json& inNode, const std::string& inKey) -> bool {
		if (!inNode.contains(inKey) || !inNode[inKey].is_string() || inNode[inKey].get<std::string>().empty())
		{
			auto path = Commands::which(inKey);
			bool res = !path.empty();
			inNode[inKey] = std::move(path);
			m_jsonFile.setDirty(true);
			return res;
		}

		return true;
	};
	whichAdd(toolchains, kKeyCmake);

	if (!toolchains.contains(kKeyMake) || !toolchains[kKeyMake].is_string() || toolchains[kKeyMake].get<std::string>().empty())
	{
#if defined(CHALET_WIN32)
		std::string make;

		// jom.exe - Qt's parallel NMAKE
		// nmake.exe - MSVC's make-ish build tool, alternative to MSBuild
		StringList searches;
		if (toolchain.type != ToolchainType::MSVC)
		{
			searches.emplace_back("mingw32-make");
		}
		else if (toolchain.type == ToolchainType::MSVC)
		{
			searches.emplace_back("jom");
			searches.emplace_back("nmake");
		}
		searches.push_back(kKeyMake);
		for (const auto& search : searches)
		{
			make = Commands::which(search);
			if (!make.empty())
			{
				break;
			}
		}
#else
		std::string make = Commands::which(kKeyMake);
#endif

		toolchains[kKeyMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(toolchains, kKeyNinja);

	if (!toolchains.contains(kKeyObjdump) || !toolchains[kKeyObjdump].is_string() || toolchains[kKeyObjdump].get<std::string>().empty())
	{
		std::string objdump;
		StringList searches;
		if (toolchain.type == ToolchainType::LLVM)
		{
			searches.emplace_back("llvm-objdump");
		}
		searches.push_back(kKeyObjdump);

		for (const auto& search : searches)
		{
			objdump = Commands::which(search);
			if (!objdump.empty())
				break;
		}

		toolchains[kKeyObjdump] = std::move(objdump);
		m_jsonFile.setDirty(true);
	}

	if (toolchains[kKeyStrategy].get<std::string>().empty())
	{
		auto make = toolchains.at(kKeyMake).get<std::string>();
		auto ninja = toolchains.at(kKeyNinja).get<std::string>();
		bool notNative = toolchain.strategy != StrategyType::Native;

		// Note: this is only for validation. it gets changed later
		if (!ninja.empty() && (toolchain.strategy == StrategyType::Ninja || (notNative && make.empty())))
		{
			toolchains[kKeyStrategy] = "ninja";
		}
		else if (!make.empty() && (toolchain.strategy == StrategyType::Makefile || (notNative && ninja.empty())))
		{
			toolchains[kKeyStrategy] = "makefile";
		}
		else if (toolchain.strategy == StrategyType::Native || (make.empty() && ninja.empty()))
		{
			toolchains[kKeyStrategy] = "native-experimental";
		}

		m_jsonFile.setDirty(true);
	}

	return result;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::parseToolchain(Json& inNode)
{
	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyStrategy))
		m_state.toolchain.setStrategy(val);

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyArchiver))
		m_state.toolchain.setArchiver(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCpp))
		m_state.toolchain.setCpp(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCc))
		m_state.toolchain.setCc(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyLinker))
		m_state.toolchain.setLinker(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyProfiler))
		m_state.toolchain.setProfiler(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyWindowsResource))
		m_state.toolchain.setRc(std::move(val));

	//

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCmake))
		m_state.toolchain.setCmake(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyMake))
		m_state.toolchain.setMake(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyNinja))
		m_state.toolchain.setNinja(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyObjdump))
		m_state.toolchain.setObjdump(std::move(val));

#if defined(CHALET_WIN32)
	bool checkForMsvc = m_inputs.toolchainPreference().type == ToolchainType::Unknown;
	m_state.toolchain.detectToolchainFromPaths();

	if (checkForMsvc && m_inputs.toolchainPreference().type == ToolchainType::MSVC)
	{
		if (!m_state.msvcEnvironment.create())
			return false;
	}
#else
	m_state.toolchain.detectToolchainFromPaths();
#endif

	return true;
}
}
