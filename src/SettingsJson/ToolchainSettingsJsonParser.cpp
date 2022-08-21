/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/ToolchainSettingsJsonParser.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
ToolchainSettingsJsonParser::ToolchainSettingsJsonParser(BuildState& inState, JsonFile& inJsonFile) :
	m_state(inState),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::serialize()
{
	Output::setShowCommandOverride(false);

	auto& rootNode = m_jsonFile.json;
	const auto& preferenceName = m_state.inputs.toolchainPreferenceName();
	auto& toolchains = rootNode["toolchains"];
	bool containsPref = toolchains.contains(preferenceName);

	if (!containsPref && !m_state.inputs.isToolchainPreset())
	{
		Diagnostic::error("{}: The requested toolchain of '{}' was not a recognized name or preset.", m_jsonFile.filename(), preferenceName);
		return false;
	}

	if (!containsPref)
		toolchains[preferenceName] = JsonDataType::object;

	auto& node = toolchains.at(preferenceName);
	if (!serialize(node))
		return false;

	Output::setShowCommandOverride(true);

	return true;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::serialize(Json& inNode)
{
	if (!inNode.is_object())
		return false;

	if (!makeToolchain(inNode, m_state.inputs.toolchainPreference()))
		return false;

	if (!parseToolchain(inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::validatePaths()
{
	// TODO: move
	bool result = true;
	const auto& compilerCpp = m_state.toolchain.compilerCpp().path;
	const auto& compilerC = m_state.toolchain.compilerC().path;
	const auto& archiver = m_state.toolchain.archiver();
	const auto& linker = m_state.toolchain.linker();

	if (compilerCpp.empty() || !Commands::pathExists(compilerCpp))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (compilerC.empty() || !Commands::pathExists(compilerC))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's C compiler was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (archiver.empty() || !Commands::pathExists(archiver))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's archive utility was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (linker.empty() || !Commands::pathExists(linker))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's linker was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

#if defined(CHALET_WIN32)
	const auto& compilerWindowsResource = m_state.toolchain.compilerWindowsResource();
	if (compilerWindowsResource.empty() || !Commands::pathExists(compilerWindowsResource))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::warn("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename());
	}
#endif
	if (!result)
	{
		auto& preference = m_state.inputs.toolchainPreferenceName();
		Diagnostic::error("{}: The requested toolchain of '{}' could either not be detected from {}, or contained invalid tools.", m_jsonFile.filename(), preference, Environment::getPathKey());
	}

	return result;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::makeToolchain(Json& toolchain, const ToolchainPreference& preference)
{
	if (!toolchain.contains(Keys::ToolchainVersion) || !toolchain[Keys::ToolchainVersion].is_string() || toolchain[Keys::ToolchainVersion].get<std::string>().empty())
	{
		toolchain[Keys::ToolchainVersion] = std::string();
	}

	if (!toolchain.contains(Keys::ToolchainBuildStrategy) || !toolchain[Keys::ToolchainBuildStrategy].is_string() || toolchain[Keys::ToolchainBuildStrategy].get<std::string>().empty())
	{
		toolchain[Keys::ToolchainBuildStrategy] = std::string();
	}

	if (!toolchain.contains(Keys::ToolchainBuildPathStyle) || !toolchain[Keys::ToolchainBuildPathStyle].is_string() || toolchain[Keys::ToolchainBuildPathStyle].get<std::string>().empty())
	{
		toolchain[Keys::ToolchainBuildPathStyle] = std::string();
	}

	bool isLLVM = preference.type == ToolchainType::LLVM || preference.type == ToolchainType::AppleLLVM || preference.type == ToolchainType::IntelLLVM;
	bool isGNU = preference.type == ToolchainType::GNU || preference.type == ToolchainType::MingwGNU;

	std::string cpp;
	std::string cc;
	if (!toolchain.contains(Keys::ToolchainCompilerCpp))
	{
		// auto varCXX = Environment::get("CXX");
		// if (varCXX != nullptr)
		// 	cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
			cpp = Commands::which(preference.cpp);
		}

		toolchain[Keys::ToolchainCompilerCpp] = cpp;
		m_jsonFile.setDirty(true);
	}
	if (!toolchain.contains(Keys::ToolchainCompilerC) || !toolchain[Keys::ToolchainCompilerC].is_string() || toolchain[Keys::ToolchainCompilerC].get<std::string>().empty())
	{
		// auto varCC = Environment::get("CC");
		// if (varCC != nullptr)
		// 	cc = Commands::which(varCC);

		if (cc.empty())
		{
			cc = Commands::which(preference.cc);
		}

		toolchain[Keys::ToolchainCompilerC] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(Keys::ToolchainCompilerWindowsResource) || !toolchain[Keys::ToolchainCompilerWindowsResource].is_string() || toolchain[Keys::ToolchainCompilerWindowsResource].get<std::string>().empty())
	{
		std::string rc;
		StringList searches;
		searches.push_back(preference.rc);
		if (isGNU)
		{
			searches.push_back("windres");
		}

		for (const auto& search : searches)
		{
			rc = Commands::which(search);
			if (!rc.empty())
				break;
		}

		toolchain[Keys::ToolchainCompilerWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(Keys::ToolchainLinker) || !toolchain[Keys::ToolchainLinker].is_string() || toolchain[Keys::ToolchainLinker].get<std::string>().empty())
	{
		std::string link;
		StringList searches;
		if (isLLVM)
		{
			searches.push_back(preference.linker); // lld
			searches.emplace_back("lld-link");
			searches.emplace_back("llvm-ld");
			searches.emplace_back("ld");
		}
		else if (isGNU)
		{
			searches.push_back(preference.linker);
			searches.push_back("ld");
		}
		else
		{
			searches.push_back(preference.linker);
		}

		for (const auto& search : searches)
		{
			link = Commands::which(search);
			if (!link.empty())
				break;
		}

#if defined(CHALET_WIN32)
		// handles edge case w/ MSVC & MinGW in same path
		if (preference.type == ToolchainType::VisualStudio)
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

		toolchain[Keys::ToolchainLinker] = std::move(link);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(Keys::ToolchainArchiver) || !toolchain[Keys::ToolchainArchiver].is_string() || toolchain[Keys::ToolchainArchiver].get<std::string>().empty())
	{
		std::string ar;
		StringList searches;
		if (isLLVM)
		{
			searches.emplace_back("llvm-ar");
		}
		else if (isGNU)
		{
			std::string tmp = preference.archiver;
			String::replaceAll(tmp, "gcc-", "");
			searches.emplace_back(std::move(tmp));
			searches.push_back(preference.archiver);
		}

#if defined(CHALET_MACOS)
		if (isLLVM || isGNU)
		{
			searches.emplace_back("libtool");
		}
#endif
		if (!isGNU)
			searches.push_back(preference.archiver);

		for (const auto& search : searches)
		{
			ar = Commands::which(search);
			if (!ar.empty())
				break;
		}

		toolchain[Keys::ToolchainArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(Keys::ToolchainProfiler) || !toolchain[Keys::ToolchainProfiler].is_string() || toolchain[Keys::ToolchainProfiler].get<std::string>().empty())
	{
		std::string prof;
		StringList searches;

		if (isGNU)
		{
			searches.push_back(preference.profiler);
			searches.push_back("gprof");
		}
		else
		{
			searches.push_back(preference.profiler);
		}

		for (const auto& search : searches)
		{
			prof = Commands::which(search);
			if (!prof.empty())
				break;
		}

		toolchain[Keys::ToolchainProfiler] = std::move(prof);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(Keys::ToolchainDisassembler) || !toolchain[Keys::ToolchainDisassembler].is_string() || toolchain[Keys::ToolchainDisassembler].get<std::string>().empty())
	{
		std::string disasm;
		StringList searches;
		if (isLLVM)
		{
			searches.emplace_back("llvm-objdump");
			searches.push_back(preference.disassembler);
		}
		else if (isGNU)
		{
			searches.push_back(preference.disassembler);
			searches.push_back("objdump");
		}
		else
		{
			searches.push_back(preference.disassembler);
		}

		for (const auto& search : searches)
		{
			disasm = Commands::which(search);
			if (!disasm.empty())
				break;
		}

		toolchain[Keys::ToolchainDisassembler] = std::move(disasm);
		m_jsonFile.setDirty(true);
	}

	// if (!result)
	// {
	// 	toolchain.erase(Keys::ToolchainCompilerCpp);
	// 	toolchain.erase(Keys::ToolchainCompilerC);
	// 	toolchain.erase(Keys::ToolchainLinker);
	// 	toolchain.erase(Keys::ToolchainArchiver);
	// 	toolchain.erase(Keys::ToolchainCompilerWindowsResource);
	// }

	auto whichAdd = [&](Json& inNode, const std::string& inKey) -> bool {
		if (!inNode.contains(inKey) || !inNode.at(inKey).is_string() || inNode.at(inKey).get<std::string>().empty())
		{
			auto path = Commands::which(inKey);
			bool res = !path.empty();
			if (res || !inNode[inKey].is_string() || inNode.at(inKey).get<std::string>().empty())
			{
				inNode[inKey] = std::move(path);
				m_jsonFile.setDirty(true);
			}
			return res;
		}

		return true;
	};
	whichAdd(toolchain, Keys::ToolchainCMake);

	if (!toolchain.contains(Keys::ToolchainMake) || !toolchain[Keys::ToolchainMake].is_string() || toolchain[Keys::ToolchainMake].get<std::string>().empty())
	{
#if defined(CHALET_WIN32)
		std::string make;

		// jom.exe - Qt's parallel NMAKE
		// nmake.exe - MSVC's make-ish build tool, alternative to MSBuild
		StringList searches;
		if (preference.type == ToolchainType::VisualStudio)
		{
			searches.emplace_back("jom");
			searches.emplace_back("nmake");
		}
		else
		{
			searches.emplace_back("mingw32-make");
		}

		searches.push_back(Keys::ToolchainMake);
		for (const auto& search : searches)
		{
			make = Commands::which(search);
			if (!make.empty())
			{
				break;
			}
		}
#else
		std::string make = Commands::which(Keys::ToolchainMake);
#endif

		toolchain[Keys::ToolchainMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(toolchain, Keys::ToolchainNinja);

	const auto& strategyFromInput = m_state.inputs.buildStrategyPreference();
	if (!strategyFromInput.empty() && !m_state.toolchain.strategyIsValid(strategyFromInput))
	{
		Diagnostic::error("Invalid toolchain build strategy type: {}", strategyFromInput);
		return false;
	}

	if (toolchain[Keys::ToolchainBuildStrategy].get<std::string>().empty())
	{
		auto make = toolchain.at(Keys::ToolchainMake).get<std::string>();
		auto ninja = toolchain.at(Keys::ToolchainNinja).get<std::string>();
		bool notNative = preference.strategy != StrategyType::Native;

		// Note: this is only for validation. it gets changed later
		// Note: MSBuild strategy on windows is opt-in, so we don't set it
		//   at all here unless it comes from user input
		//
		if (!strategyFromInput.empty())
		{
			toolchain[Keys::ToolchainBuildStrategy] = strategyFromInput;
		}

		else if (!ninja.empty() && (preference.strategy == StrategyType::Ninja || (notNative && make.empty())))
		{
			toolchain[Keys::ToolchainBuildStrategy] = "ninja";
		}
		else if (!make.empty() && (preference.strategy == StrategyType::Makefile || (notNative && ninja.empty())))
		{
			toolchain[Keys::ToolchainBuildStrategy] = "makefile";
		}
		else if (preference.strategy == StrategyType::Native || (make.empty() && ninja.empty()))
		{
			toolchain[Keys::ToolchainBuildStrategy] = "native-experimental";
		}

		m_jsonFile.setDirty(true);
	}
	else if (!strategyFromInput.empty())
	{
		toolchain[Keys::ToolchainBuildStrategy] = strategyFromInput;

		m_jsonFile.setDirty(true);
	}

	const auto& buildPathStyleFromInput = m_state.inputs.buildPathStylePreference();
	if (!buildPathStyleFromInput.empty() && !m_state.toolchain.buildPathStyleIsValid(buildPathStyleFromInput))
	{
		Diagnostic::error("Invalid toolchain build path style type: {}", buildPathStyleFromInput);
		return false;
	}

	if (toolchain[Keys::ToolchainBuildPathStyle].get<std::string>().empty())
	{
		// Note: this is only for validation. it gets changed later
		if (!buildPathStyleFromInput.empty())
		{
			toolchain[Keys::ToolchainBuildPathStyle] = buildPathStyleFromInput;
		}

		else if (preference.buildPathStyle == BuildPathStyle::TargetTriple)
		{
			toolchain[Keys::ToolchainBuildPathStyle] = "target-triple";
		}
		else if (preference.buildPathStyle == BuildPathStyle::ToolchainName)
		{
			toolchain[Keys::ToolchainBuildPathStyle] = "toolchain-name";
		}
		else if (preference.buildPathStyle == BuildPathStyle::Configuration)
		{
			toolchain[Keys::ToolchainBuildPathStyle] = "configuration";
		}
		else if (preference.buildPathStyle == BuildPathStyle::ArchConfiguration)
		{
			toolchain[Keys::ToolchainBuildPathStyle] = "architecture";
		}

		m_jsonFile.setDirty(true);
	}
	else if (!buildPathStyleFromInput.empty())
	{
		toolchain[Keys::ToolchainBuildPathStyle] = buildPathStyleFromInput;
		m_jsonFile.setDirty(true);
	}

	if (toolchain[Keys::ToolchainVersion].get<std::string>().empty())
	{
		toolchain[Keys::ToolchainVersion] = std::string();
	}

	return true;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::parseToolchain(Json& inNode)
{
	StringList removeKeys;
	for (const auto& [key, value] : inNode.items())
	{
		if (value.is_string())
		{
			if (String::equals(Keys::ToolchainBuildStrategy, key))
				m_state.toolchain.setStrategy(value.get<std::string>());
			else if (String::equals(Keys::ToolchainBuildPathStyle, key))
				m_state.toolchain.setBuildPathStyle(value.get<std::string>());
			else if (String::equals(Keys::ToolchainVersion, key))
				m_state.toolchain.setVersion(value.get<std::string>());
			else if (String::equals(Keys::ToolchainArchiver, key))
				m_state.toolchain.setArchiver(value.get<std::string>());
			else if (String::equals(Keys::ToolchainCompilerCpp, key))
				m_state.toolchain.setCompilerCpp(value.get<std::string>());
			else if (String::equals(Keys::ToolchainCompilerC, key))
				m_state.toolchain.setCompilerC(value.get<std::string>());
			else if (String::equals(Keys::ToolchainCompilerWindowsResource, key))
				m_state.toolchain.setCompilerWindowsResource(value.get<std::string>());
			else if (String::equals(Keys::ToolchainLinker, key))
				m_state.toolchain.setLinker(value.get<std::string>());
			else if (String::equals(Keys::ToolchainProfiler, key))
				m_state.toolchain.setProfiler(value.get<std::string>());
			//
			else if (String::equals(Keys::ToolchainCMake, key))
				m_state.toolchain.setCmake(value.get<std::string>());
			else if (String::equals(Keys::ToolchainMake, key))
				m_state.toolchain.setMake(value.get<std::string>());
			else if (String::equals(Keys::ToolchainNinja, key))
				m_state.toolchain.setNinja(value.get<std::string>());
			else if (String::equals(Keys::ToolchainDisassembler, key))
				m_state.toolchain.setDisassembler(value.get<std::string>());
			else
				removeKeys.push_back(key);
		}
	}

	for (auto& key : removeKeys)
	{
		inNode.erase(key);
	}

	if (!removeKeys.empty())
		m_jsonFile.setDirty(true);

	return true;
}
}
