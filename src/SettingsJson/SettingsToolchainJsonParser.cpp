/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/SettingsToolchainJsonParser.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
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

	m_checkForEnvironment = false;
	auto& preference = m_inputs.toolchainPreference();
	if (preference.type != ToolchainType::Unknown)
	{
		m_state.environment = ICompileEnvironment::make(preference.type, m_inputs, m_state);
		if (m_state.environment == nullptr)
			return false;

		if (!m_state.environment->create())
			return false;
	}
	else
	{
		m_checkForEnvironment = true;
	}

	auto& rootNode = m_jsonFile.json;
	const auto& preferenceName = m_inputs.toolchainPreferenceName();
	auto& toolchains = rootNode["toolchains"];
	bool containsPref = toolchains.contains(preferenceName);

	if (!containsPref && !m_inputs.isToolchainPreset())
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
bool SettingsToolchainJsonParser::serialize(Json& inNode)
{
	if (!inNode.is_object())
		return false;

	if (!makeToolchain(inNode, m_inputs.toolchainPreference()))
		return false;

	if (!parseToolchain(inNode))
		return false;

	if (!finalizeEnvironment())
		return false;

	if (!validatePaths())
		return false;

	return true;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::validatePaths()
{
	bool result = true;
	if (m_state.toolchain.compilerCpp().empty() || !Commands::pathExists(m_state.toolchain.compilerCpp()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename());
		result = false;
	}

	if (m_state.toolchain.compilerC().empty() || !Commands::pathExists(m_state.toolchain.compilerC()))
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
	if (m_state.toolchain.compilerWindowsResource().empty() || !Commands::pathExists(m_state.toolchain.compilerWindowsResource()))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::warn("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename());
	}
#endif
	if (!result)
	{
		auto& preference = m_inputs.toolchainPreferenceName();
		Diagnostic::error("{}: The requested toolchain of '{}' could either not be detected from {}, or contained invalid tools.", m_jsonFile.filename(), preference, Environment::getPathKey());
	}

	return result;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::makeToolchain(Json& toolchain, const ToolchainPreference& preference)
{
	if (!toolchain.contains(kKeyVersion) || !toolchain[kKeyVersion].is_string() || toolchain[kKeyVersion].get<std::string>().empty())
	{
		toolchain[kKeyVersion] = std::string();
	}

	if (!toolchain.contains(kKeyStrategy) || !toolchain[kKeyStrategy].is_string() || toolchain[kKeyStrategy].get<std::string>().empty())
	{
		toolchain[kKeyStrategy] = std::string();
	}

	if (!toolchain.contains(kKeyBuildPathStyle) || !toolchain[kKeyBuildPathStyle].is_string() || toolchain[kKeyBuildPathStyle].get<std::string>().empty())
	{
		toolchain[kKeyBuildPathStyle] = std::string();
	}

	bool isLLVM = preference.type == ToolchainType::LLVM || preference.type == ToolchainType::AppleLLVM || preference.type == ToolchainType::IntelLLVM;

	std::string cpp;
	std::string cc;
	if (!toolchain.contains(kKeyCompilerCpp))
	{
		// auto varCXX = Environment::get("CXX");
		// if (varCXX != nullptr)
		// 	cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
			cpp = Commands::which(preference.cpp);
		}

		toolchain[kKeyCompilerCpp] = cpp;
		m_jsonFile.setDirty(true);
	}
	if (!toolchain.contains(kKeyCompilerC) || !toolchain[kKeyCompilerC].is_string() || toolchain[kKeyCompilerC].get<std::string>().empty())
	{
		// auto varCC = Environment::get("CC");
		// if (varCC != nullptr)
		// 	cc = Commands::which(varCC);

		if (cc.empty())
		{
			cc = Commands::which(preference.cc);
		}

		toolchain[kKeyCompilerC] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(kKeyCompilerWindowsResource) || !toolchain[kKeyCompilerWindowsResource].is_string() || toolchain[kKeyCompilerWindowsResource].get<std::string>().empty())
	{
		std::string rc;
		StringList searches;
		searches.push_back(preference.rc);

		for (const auto& search : searches)
		{
			rc = Commands::which(search);
			if (!rc.empty())
				break;
		}

		toolchain[kKeyCompilerWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(kKeyLinker) || !toolchain[kKeyLinker].is_string() || toolchain[kKeyLinker].get<std::string>().empty())
	{
		std::string link;
		StringList searches;
		if (isLLVM)
		{
			searches.push_back(preference.linker); // lld
			searches.emplace_back("llvm-link");
			searches.emplace_back("llvm-ld");
			searches.emplace_back("ld");
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

		toolchain[kKeyLinker] = std::move(link);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(kKeyArchiver) || !toolchain[kKeyArchiver].is_string() || toolchain[kKeyArchiver].get<std::string>().empty())
	{
		std::string ar;
		StringList searches;
		if (isLLVM)
		{
			searches.emplace_back("llvm-ar");
		}
#if defined(CHALET_MACOS)
		if (isLLVM || preference.type == ToolchainType::GNU)
		{
			searches.emplace_back("libtool");
		}
#endif
		searches.push_back(preference.archiver);

		for (const auto& search : searches)
		{
			ar = Commands::which(search);
			if (!ar.empty())
				break;
		}

		toolchain[kKeyArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(kKeyProfiler) || !toolchain[kKeyProfiler].is_string() || toolchain[kKeyProfiler].get<std::string>().empty())
	{
		std::string prof;
		StringList searches;
		searches.push_back(preference.profiler);

		for (const auto& search : searches)
		{
			prof = Commands::which(search);
			if (!prof.empty())
				break;
		}

		toolchain[kKeyProfiler] = std::move(prof);
		m_jsonFile.setDirty(true);
	}

	if (!toolchain.contains(kKeyDisassembler) || !toolchain[kKeyDisassembler].is_string() || toolchain[kKeyDisassembler].get<std::string>().empty())
	{
		std::string disasm;
		StringList searches;
		if (isLLVM)
		{
			searches.emplace_back("llvm-objdump");
		}
		searches.push_back(preference.disassembler);

		for (const auto& search : searches)
		{
			disasm = Commands::which(search);
			if (!disasm.empty())
				break;
		}

		toolchain[kKeyDisassembler] = std::move(disasm);
		m_jsonFile.setDirty(true);
	}

	// if (!result)
	// {
	// 	toolchain.erase(kKeyCompilerCpp);
	// 	toolchain.erase(kKeyCompilerC);
	// 	toolchain.erase(kKeyLinker);
	// 	toolchain.erase(kKeyArchiver);
	// 	toolchain.erase(kKeyCompilerWindowsResource);
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
	whichAdd(toolchain, kKeyCmake);

	if (!toolchain.contains(kKeyMake) || !toolchain[kKeyMake].is_string() || toolchain[kKeyMake].get<std::string>().empty())
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

		toolchain[kKeyMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(toolchain, kKeyNinja);

	if (toolchain[kKeyStrategy].get<std::string>().empty())
	{
		auto make = toolchain.at(kKeyMake).get<std::string>();
		auto ninja = toolchain.at(kKeyNinja).get<std::string>();
		bool notNative = preference.strategy != StrategyType::Native;

		// Note: this is only for validation. it gets changed later
		if (!ninja.empty() && (preference.strategy == StrategyType::Ninja || (notNative && make.empty())))
		{
			toolchain[kKeyStrategy] = "ninja";
		}
		else if (!make.empty() && (preference.strategy == StrategyType::Makefile || (notNative && ninja.empty())))
		{
			toolchain[kKeyStrategy] = "makefile";
		}
		else if (preference.strategy == StrategyType::Native || (make.empty() && ninja.empty()))
		{
			toolchain[kKeyStrategy] = "native-experimental";
		}

		m_jsonFile.setDirty(true);
	}

	if (toolchain[kKeyBuildPathStyle].get<std::string>().empty())
	{
		// Note: this is only for validation. it gets changed later
		if (preference.buildPathStyle == BuildPathStyle::TargetTriple)
		{
			toolchain[kKeyBuildPathStyle] = "target-triple";
		}
		else if (preference.buildPathStyle == BuildPathStyle::ToolchainName)
		{
			toolchain[kKeyBuildPathStyle] = "toolchain-name";
		}
		else if (preference.buildPathStyle == BuildPathStyle::Configuration)
		{
			toolchain[kKeyBuildPathStyle] = "configuration";
		}
		else if (preference.buildPathStyle == BuildPathStyle::ArchConfiguration)
		{
			toolchain[kKeyBuildPathStyle] = "arch-configuration";
		}

		m_jsonFile.setDirty(true);
	}

	if (toolchain[kKeyVersion].get<std::string>().empty())
	{
		toolchain[kKeyVersion] = m_state.environment != nullptr ? m_state.environment->detectedVersion() : std::string();
	}

	return true;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::parseToolchain(Json& inNode)
{
	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyStrategy))
		m_state.toolchain.setStrategy(val);

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyBuildPathStyle))
		m_state.toolchain.setBuildPathStyle(val);

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyVersion))
		m_state.toolchain.setVersion(val);

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyArchiver))
		m_state.toolchain.setArchiver(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCompilerCpp))
		m_state.toolchain.setCompilerCpp(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCompilerC))
		m_state.toolchain.setCompilerC(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCompilerWindowsResource))
		m_state.toolchain.setCompilerWindowsResource(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyLinker))
		m_state.toolchain.setLinker(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyProfiler))
		m_state.toolchain.setProfiler(std::move(val));

	//

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCmake))
		m_state.toolchain.setCmake(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyMake))
		m_state.toolchain.setMake(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyNinja))
		m_state.toolchain.setNinja(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyDisassembler))
		m_state.toolchain.setDisassembler(std::move(val));

	return true;
}

/*****************************************************************************/
bool SettingsToolchainJsonParser::finalizeEnvironment()
{
	auto& preference = m_inputs.toolchainPreference();
	ToolchainType type = ICompileEnvironment::detectToolchainTypeFromPath(m_state.toolchain.compilerCxx());
	if (preference.type != ToolchainType::Unknown && preference.type != type)
	{
		Diagnostic::error("Could not find a suitable toolchain that matches '{}'. Try configuring one manually.", m_inputs.toolchainPreferenceName());
		return false;
	}

	if (m_checkForEnvironment)
	{
		m_state.environment = ICompileEnvironment::make(preference.type, m_inputs, m_state);
		if (m_state.environment == nullptr)
			return false;

		if (!m_state.environment->create(m_state.toolchain.version()))
			return false;
	}

	if (m_state.toolchain.version().empty())
		m_state.toolchain.setVersion(m_state.environment->detectedVersion());

	return true;
}
}
