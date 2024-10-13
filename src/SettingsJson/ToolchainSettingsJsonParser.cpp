/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "SettingsJson/ToolchainSettingsJsonParser.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
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

	auto onError = []() {
		Output::setShowCommandOverride(true);
		return false;
	};

	auto& jRoot = m_jsonFile.root;
	const auto& preferenceName = m_state.inputs.toolchainPreferenceName();

	auto& toolchains = jRoot[Keys::Toolchains];

	auto envToolchainName = Environment::getString("CHALET_TOOLCHAIN_NAME");
	m_isCustomToolchain = !envToolchainName.empty() && String::equals(preferenceName, envToolchainName);
	if (m_isCustomToolchain)
	{
		if (!m_state.inputs.makeCustomToolchainFromEnvironment())
		{
			Diagnostic::error("{}: The requested toolchain of '{}' could not be detected due to an internal error.", m_jsonFile.filename(), preferenceName);
			return onError();
		}
	}
	else
	{
		bool containsPref = toolchains.contains(preferenceName);
		if (!containsPref && !m_state.inputs.isToolchainPreset())
		{
			Diagnostic::error("{}: The requested toolchain of '{}' was not a recognized name or preset.", m_jsonFile.filename(), preferenceName);
			return onError();
		}
	}

	auto& toolchainNode = getToolchainNode(toolchains);
	if (!serialize(toolchainNode))
		return onError();

	Output::setShowCommandOverride(true);
	return true;
}

/*****************************************************************************/
// This is called from BuildState::checkForExceptionalToolchainCases()
//   If a (Visual Studio) toolchain no longer exists, we want to refresh it
//
bool ToolchainSettingsJsonParser::validatePathsWithoutFullParseAndEraseToolchainOnFailure()
{
	bool result = true;

	Output::setShowCommandOverride(false);

	auto& jRoot = m_jsonFile.root;

	auto& toolchains = jRoot[Keys::Toolchains];
	if (!toolchains.empty())
	{
		auto& toolchain = getToolchainNode(toolchains);
		if (!toolchain.empty())
		{
			auto pathFromKeyIsValid = [&toolchain](const char* inKey) {
				auto value = json::get<std::string>(toolchain, inKey);
				return !value.empty() && Files::pathExists(value);
			};

			// We only want to wipe the toolchain if ALL of these are not found

			result = false;
			result |= pathFromKeyIsValid(Keys::ToolchainCompilerCpp);
			result |= pathFromKeyIsValid(Keys::ToolchainCompilerC);
			result |= pathFromKeyIsValid(Keys::ToolchainLinker);
			result |= pathFromKeyIsValid(Keys::ToolchainArchiver);
			result |= pathFromKeyIsValid(Keys::ToolchainDisassembler);

			if (!result)
			{
				// Wipe the toolchain
				const auto& preferenceName = m_state.inputs.toolchainPreferenceName();
				toolchains.erase(preferenceName);
			}
		}
	}

	Output::setShowCommandOverride(true);
	return result;
}

/*****************************************************************************/
Json& ToolchainSettingsJsonParser::getToolchainNode(Json& inToolchainsNode)
{
	const auto& preferenceName = m_state.inputs.toolchainPreferenceName();
	auto arch = m_state.inputs.getResolvedTargetArchitecture();

	if (!json::isObject(inToolchainsNode, preferenceName.c_str()))
		inToolchainsNode[preferenceName] = Json::object();

	auto& node = inToolchainsNode[preferenceName];
	bool isDefined = false;
	StringList keys{
		Keys::ToolchainCompilerC,
		Keys::ToolchainCompilerCpp,
		Keys::ToolchainLinker,
		Keys::ToolchainArchiver,
		Keys::ToolchainDisassembler,
		Keys::ToolchainBuildStrategy,
		Keys::ToolchainBuildPathStyle,
		Keys::ToolchainCMake,
		Keys::ToolchainMake,
		Keys::ToolchainNinja,
		Keys::ToolchainProfiler,
		Keys::ToolchainVersion,
	};
	for (auto& [key, _] : node.items())
	{
		isDefined |= String::equals(keys, key);
		if (isDefined)
			break;
	}
	if (!isDefined && (m_state.inputs.isMultiArchToolchainPreset() || m_state.inputs.toolchainPreference().type == ToolchainType::Unknown))
	{
		auto arch2 = Arch::from(arch);
		if (!node.contains(arch2.str))
		{
			node[arch2.str] = Json::object();
		}
		m_state.inputs.setMultiArchToolchainPreset(true);

		return node[arch2.str];
	}
	else
	{
		return node;
	}
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::serialize(Json& inNode)
{
	if (!inNode.is_object())
		return false;

	if (!makeToolchain(inNode, m_state.inputs.toolchainPreference()))
		return false;

	// LOG(inNode.dump(1, '\t'));

	if (!parseToolchain(inNode))
		return false;

	return true;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::validatePaths()
{
	bool result = true;
	const auto& compilerCpp = m_state.toolchain.compilerCpp().path;
	const auto& compilerC = m_state.toolchain.compilerC().path;
	const auto& compilerWindowsResource = m_state.toolchain.compilerWindowsResource();
	const auto& archiver = m_state.toolchain.archiver();
	const auto& linker = m_state.toolchain.linker();

	auto pathIsInvalid = [&result](const std::string& inPath) {
		bool isInvalid = inPath.empty() || !Files::pathExists(inPath);
		if (isInvalid)
		{
			result = false;
		}
		return isInvalid;
	};

	if (pathIsInvalid(compilerCpp))
		Diagnostic::error("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename());

	if (pathIsInvalid(compilerC))
		Diagnostic::error("{}: The toolchain's C compiler was blank or could not be found.", m_jsonFile.filename());

	if (pathIsInvalid(archiver))
		Diagnostic::error("{}: The toolchain's archive utility was blank or could not be found.", m_jsonFile.filename());

	if (pathIsInvalid(linker))
		Diagnostic::error("{}: The toolchain's linker was blank or could not be found.", m_jsonFile.filename());

#if defined(CHALET_WIN32)
	if (m_state.environment != nullptr && !m_state.environment->isEmscripten())
	{
		if (pathIsInvalid(compilerWindowsResource))
			Diagnostic::warn("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename());
	}
#else
	UNUSED(compilerWindowsResource);
#endif
	if (!result)
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		auto& preference = m_state.inputs.toolchainPreferenceName();
		auto arch = m_state.inputs.getResolvedTargetArchitecture();

		Diagnostic::error("{}: The requested toolchain of '{}' (arch: {}) could either not be detected from {}, or contained invalid tools.", m_jsonFile.filename(), preference, arch, Environment::getPathKey());
	}

	return result;
}

/*****************************************************************************/
bool ToolchainSettingsJsonParser::makeToolchain(Json& toolchain, const ToolchainPreference& preference)
{
	if (!json::isValid<std::string>(toolchain, Keys::ToolchainVersion))
		toolchain[Keys::ToolchainVersion] = std::string();

	if (!json::isValid<std::string>(toolchain, Keys::ToolchainBuildStrategy))
		toolchain[Keys::ToolchainBuildStrategy] = std::string();

	if (!json::isValid<std::string>(toolchain, Keys::ToolchainBuildPathStyle))
		toolchain[Keys::ToolchainBuildPathStyle] = std::string();

	bool isLLVM = preference.type == ToolchainType::LLVM
		|| preference.type == ToolchainType::AppleLLVM
		|| preference.type == ToolchainType::IntelLLVM
		|| preference.type == ToolchainType::VisualStudioLLVM;

	bool isGNU = preference.type == ToolchainType::GNU
		|| preference.type == ToolchainType::MingwGNU;

	std::string cpp;
	std::string cc;
	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainCompilerCpp))
	{
		if (cpp.empty())
			cpp = Files::which(preference.cpp);

		toolchain[Keys::ToolchainCompilerCpp] = cpp;
		m_jsonFile.setDirty(true);
	}

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainCompilerC))
	{
		if (cc.empty())
			cc = Files::which(preference.cc);

		toolchain[Keys::ToolchainCompilerC] = cc;
		m_jsonFile.setDirty(true);
	}

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainCompilerWindowsResource))
	{
		std::string rc;
		StringList searches;

		if (m_isCustomToolchain)
		{
			if (!preference.rc.empty())
			{
				searches.push_back(preference.rc);
			}
			else
			{
#if defined(CHALET_LINUX)
				searches.push_back("llvm-windres");
#endif
				List::addIfDoesNotExist(searches, std::string("windres"));
				searches.push_back("llvm-rc"); // check this after windres
			}
		}
		else if (isGNU)
		{
			searches.push_back(preference.rc);
			List::addIfDoesNotExist(searches, std::string("windres"));
		}
		else
		{
			searches.push_back(preference.rc);
		}

		for (const auto& search : searches)
		{
			rc = Files::which(search);
			if (!rc.empty())
				break;
		}

		toolchain[Keys::ToolchainCompilerWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainLinker))
	{
		std::string link;
		StringList searches;
		if (m_isCustomToolchain)
		{
			if (!preference.linker.empty())
			{
				searches.push_back(preference.linker);
			}
			else
			{
				searches.push_back("lld");
				searches.push_back("lld-link");
				searches.push_back("llvm-link");
				searches.push_back("llvm-ld");
				searches.push_back("ld");
			}
		}
		else if (isLLVM)
		{
			std::string suffix;
			if (String::contains('-', preference.cc))
				suffix = preference.cc.substr(preference.cc.find_first_of('-'));

			searches.push_back(preference.linker); // lld
			searches.emplace_back("lld-link");
			searches.emplace_back(fmt::format("llvm-ld{}", suffix));
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
			link = Files::which(search);
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

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainArchiver))
	{
		std::string ar;
		StringList searches;
		if (m_isCustomToolchain)
		{
			if (!preference.archiver.empty())
			{
				searches.push_back(preference.archiver);
			}
			else
			{
				searches.push_back("llvm-ar");
				searches.push_back("ar");
			}
		}
		else if (isLLVM)
		{
			std::string suffix;
			if (String::contains('-', preference.cc))
				suffix = preference.cc.substr(preference.cc.find_first_of('-'));

			searches.emplace_back(fmt::format("llvm-ar{}", suffix));
		}
		else if (isGNU)
		{
			std::string tmp = preference.archiver;
			String::replaceAll(tmp, "gcc-", "");
			searches.emplace_back(std::move(tmp));
			searches.push_back(preference.archiver);
		}

#if defined(CHALET_MACOS)
		if (m_isCustomToolchain || isLLVM || isGNU)
			searches.emplace_back("libtool");
#endif

		if (!isGNU)
			searches.push_back(preference.archiver);

		for (const auto& search : searches)
		{
			ar = Files::which(search);
			if (!ar.empty())
				break;
		}

		toolchain[Keys::ToolchainArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainProfiler))
	{
		std::string prof;
		StringList searches;
		searches.push_back(preference.profiler);
		if (m_isCustomToolchain)
		{
			if (!preference.profiler.empty())
			{
				searches.push_back(preference.profiler);
			}
			else
			{
				searches.push_back("gprof");
			}
		}
		else if (isGNU)
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
			prof = Files::which(search);
			if (!prof.empty())
				break;
		}

		toolchain[Keys::ToolchainProfiler] = std::move(prof);
		m_jsonFile.setDirty(true);
	}

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainDisassembler))
	{
		std::string disasm;
		StringList searches;
		if (m_isCustomToolchain)
		{
			if (!preference.disassembler.empty())
			{
				searches.push_back(preference.disassembler);
			}
			else
			{
				searches.push_back("llvm-objdump");
#if defined(CHALET_WIN32)
				searches.push_back("dumpbin");
#elif defined(CHALET_MACOS)
				searches.push_back("otool");
#endif
				searches.push_back("objdump");
			}
		}
		else if (isLLVM)
		{
			std::string suffix;
			if (String::contains('-', preference.cc))
				suffix = preference.cc.substr(preference.cc.find_first_of('-'));

			searches.emplace_back(fmt::format("llvm-objdump{}", suffix));
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
			disasm = Files::which(search);
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

	auto whichAdd = [this](Json& inNode, const char* inKey) -> bool {
		if (json::isStringInvalidOrEmpty(inNode, inKey))
		{
			auto path = Files::which(inKey);
			bool res = !path.empty();
			if (res || !inNode[inKey].is_string() || inNode[inKey].get<std::string>().empty())
			{
				inNode[inKey] = std::move(path);
				m_jsonFile.setDirty(true);
			}
			return res;
		}

		return true;
	};
	whichAdd(toolchain, Keys::ToolchainCMake);

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainMake))
	{
#if defined(CHALET_WIN32)
		std::string make;

		// jom.exe - Qt's parallel NMAKE
		// nmake.exe - MSVC's make-ish build tool, alternative to MSBuild
		StringList searches;
		if (preference.type == ToolchainType::VisualStudio || preference.type == ToolchainType::VisualStudioLLVM)
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
			make = Files::which(search);
			if (!make.empty())
				break;
		}
#else
		std::string make = Files::which(Keys::ToolchainMake);
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
		auto make = toolchain[Keys::ToolchainMake].get<std::string>();
		auto ninja = toolchain[Keys::ToolchainNinja].get<std::string>();
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
			toolchain[Keys::ToolchainBuildStrategy] = "native";
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

	if (json::isStringInvalidOrEmpty(toolchain, Keys::ToolchainBuildPathStyle))
	{
		// Note: this is only for validation. it gets changed later
		if (!buildPathStyleFromInput.empty())
			toolchain[Keys::ToolchainBuildPathStyle] = buildPathStyleFromInput;

		else if (preference.buildPathStyle == BuildPathStyle::TargetTriple)
			toolchain[Keys::ToolchainBuildPathStyle] = "target-triple";
		else if (preference.buildPathStyle == BuildPathStyle::ToolchainName)
			toolchain[Keys::ToolchainBuildPathStyle] = "toolchain-name";
		else if (preference.buildPathStyle == BuildPathStyle::Configuration)
			toolchain[Keys::ToolchainBuildPathStyle] = "configuration";
		else if (preference.buildPathStyle == BuildPathStyle::ArchConfiguration)
			toolchain[Keys::ToolchainBuildPathStyle] = "architecture";

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
