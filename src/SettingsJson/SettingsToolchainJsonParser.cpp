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

#if defined(CHALET_WIN32)
	auto& toolchain = m_inputs.toolchainPreference();
	if (toolchain.type == ToolchainType::MSVC)
	{
		if (!m_state.msvcEnvironment.create())
			return false;
	}
#endif

	// TODO: Move
	if (String::equals("gcc", m_inputs.toolchainPreferenceName()))
	{
		std::string arch = m_state.info.targetArchitectureString();
		if (String::contains('-', arch))
		{
			auto split = String::split(arch, '-');
			arch = std::move(split.front());
		}

		m_inputs.setToolchainPreferenceName(fmt::format("{}-gcc", arch));
	}

	auto& rootNode = m_jsonFile.json;

	const auto& preference = m_inputs.toolchainPreferenceName();
	auto& toolchains = rootNode["toolchains"];
	if (!toolchains.contains(preference))
	{
		if (!m_inputs.isToolchainPreset())
		{
			Diagnostic::error("{}: The requested toolchain of '{}' was not a recognized toolchain name or preset.", m_jsonFile.filename(), preference);
			return false;
		}

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
bool SettingsToolchainJsonParser::makeToolchain(Json& toolchains, const ToolchainPreference& toolchain)
{
	bool result = true;

	if (toolchain.type == ToolchainType::Unknown)
		return result;

	if (!toolchains.contains(kKeyVersion) || !toolchains[kKeyVersion].is_string() || toolchains[kKeyVersion].get<std::string>().empty())
	{
		toolchains[kKeyVersion] = std::string();
	}

	if (!toolchains.contains(kKeyStrategy) || !toolchains[kKeyStrategy].is_string() || toolchains[kKeyStrategy].get<std::string>().empty())
	{
		toolchains[kKeyStrategy] = std::string();
	}

	if (!toolchains.contains(kKeyBuildPathStyle) || !toolchains[kKeyBuildPathStyle].is_string() || toolchains[kKeyBuildPathStyle].get<std::string>().empty())
	{
		toolchains[kKeyBuildPathStyle] = std::string();
	}

	// kKeyBuildPathStyle

	std::string cpp;
	std::string cc;
	if (!toolchains.contains(kKeyCompilerCpp))
	{
		// auto varCXX = Environment::get("CXX");
		// if (varCXX != nullptr)
		// 	cpp = Commands::which(varCXX);

		if (cpp.empty())
		{
			cpp = Commands::which(toolchain.cpp);
		}

		result &= !cpp.empty();

		toolchains[kKeyCompilerCpp] = cpp;
		m_jsonFile.setDirty(true);
	}
	if (!toolchains.contains(kKeyCompilerC) || !toolchains[kKeyCompilerC].is_string() || toolchains[kKeyCompilerC].get<std::string>().empty())
	{
		// auto varCC = Environment::get("CC");
		// if (varCC != nullptr)
		// 	cc = Commands::which(varCC);

		if (cc.empty())
		{
			cc = Commands::which(toolchain.cc);
		}

		result &= !cc.empty();

		toolchains[kKeyCompilerC] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyCompilerWindowsResource) || !toolchains[kKeyCompilerWindowsResource].is_string() || toolchains[kKeyCompilerWindowsResource].get<std::string>().empty())
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

		toolchains[kKeyCompilerWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyLinker) || !toolchains[kKeyLinker].is_string() || toolchains[kKeyLinker].get<std::string>().empty())
	{
		std::string link;
		StringList searches;
		if (toolchain.type == ToolchainType::LLVM)
		{
			searches.push_back(toolchain.linker); // lld
			searches.emplace_back("llvm-link");
			searches.emplace_back("llvm-ld");
			searches.emplace_back("ld");
		}
		else
		{
			searches.push_back(toolchain.linker);
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
#if defined(CHALET_MACOS)
		if (toolchain.type == ToolchainType::LLVM || toolchain.type == ToolchainType::GNU)
		{
			searches.emplace_back("libtool");
		}
#endif
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

	if (!toolchains.contains(kKeyDisassembler) || !toolchains[kKeyDisassembler].is_string() || toolchains[kKeyDisassembler].get<std::string>().empty())
	{
		std::string disasm;
		StringList searches;
		if (toolchain.type == ToolchainType::LLVM)
		{
			searches.emplace_back("llvm-objdump");
		}
		searches.push_back(toolchain.disassembler);

		for (const auto& search : searches)
		{
			disasm = Commands::which(search);
			if (!disasm.empty())
				break;
		}

		toolchains[kKeyDisassembler] = std::move(disasm);
		m_jsonFile.setDirty(true);
	}

	// if (!result)
	// {
	// 	toolchains.erase(kKeyCompilerCpp);
	// 	toolchains.erase(kKeyCompilerC);
	// 	toolchains.erase(kKeyLinker);
	// 	toolchains.erase(kKeyArchiver);
	// 	toolchains.erase(kKeyCompilerWindowsResource);
	// }

	auto whichAdd = [&](Json& inNode, const std::string& inKey) -> bool {
		if (!inNode.contains(inKey) || !inNode[inKey].is_string() || inNode[inKey].get<std::string>().empty())
		{
			auto path = Commands::which(inKey);
			bool res = !path.empty();
			if (res)
			{
				inNode[inKey] = std::move(path);
				m_jsonFile.setDirty(true);
			}
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
		if (toolchain.type == ToolchainType::MSVC)
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

		toolchains[kKeyMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(toolchains, kKeyNinja);

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

	if (toolchains[kKeyBuildPathStyle].get<std::string>().empty())
	{
		// Note: this is only for validation. it gets changed later
		if (toolchain.buildPathStyle == BuildPathStyle::TargetTriple)
		{
			toolchains[kKeyBuildPathStyle] = "target-triple";
		}
		else if (toolchain.buildPathStyle == BuildPathStyle::ToolchainName)
		{
			toolchains[kKeyBuildPathStyle] = "toolchain-name";
		}
		else if (toolchain.buildPathStyle == BuildPathStyle::Configuration)
		{
			toolchains[kKeyBuildPathStyle] = "configuration";
		}
		else if (toolchain.buildPathStyle == BuildPathStyle::ArchConfiguration)
		{
			toolchains[kKeyBuildPathStyle] = "arch-configuration";
		}

		m_jsonFile.setDirty(true);
	}

	if (toolchains[kKeyVersion].get<std::string>().empty())
	{
#if defined(CHALET_WIN32)
		// Only used w/ MSVC for now
		auto& vsVersion = m_state.msvcEnvironment.detectedVersion();
		toolchains[kKeyVersion] = !vsVersion.empty() ? vsVersion : std::string();
#else
		toolchains[kKeyVersion] = std::string();
#endif
	}

	return result;
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

#if defined(CHALET_WIN32)
	bool checkForMsvc = m_inputs.toolchainPreference().type == ToolchainType::Unknown;
	if (!m_state.toolchain.detectToolchainFromPaths())
		return false;

	if (m_inputs.toolchainPreference().type == ToolchainType::MSVC)
	{
		if (checkForMsvc)
		{
			if (!m_state.msvcEnvironment.create(m_state.toolchain.version()))
				return false;
		}

		if (m_state.toolchain.version().empty())
			m_state.toolchain.setVersion(m_state.msvcEnvironment.detectedVersion());
	}
#else
	m_state.toolchain.detectToolchainFromPaths();
#endif

	return true;
}
}
