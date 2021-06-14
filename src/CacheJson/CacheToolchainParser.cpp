/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "CacheJson/CacheToolchainParser.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
/*****************************************************************************/
CacheToolchainParser::CacheToolchainParser(const CommandLineInputs& inInputs, BuildState& inState, JsonFile& inJsonFile) :
	m_inputs(inInputs),
	m_state(inState),
	m_jsonFile(inJsonFile)
{
}

/*****************************************************************************/
bool CacheToolchainParser::serialize()
{
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
	return serialize(node);
}

/*****************************************************************************/
bool CacheToolchainParser::serialize(Json& inNode)
{
	if (!inNode.is_object())
		return false;

#if defined(CHALET_WIN32)
	if (!createMsvcEnvironment(inNode))
		return false;
#endif

	auto& toolchain = m_inputs.toolchainPreference();
#if defined(CHALET_WIN32)
	if (!makeToolchain(inNode, toolchain))
	{
		if (toolchain.type == ToolchainType::MSVC)
		{
			m_inputs.setToolchainPreference("gcc"); // aka mingw
			if (!makeToolchain(inNode, toolchain))
			{
				m_inputs.setToolchainPreference("llvm"); // try once more for clang
				makeToolchain(inNode, toolchain);
			}
		}
	}
#else
	makeToolchain(inNode, toolchain);
#endif

	if (!parseToolchain(inNode))
		return false;

	if (!validatePaths())
		return false;

	return true;
}

/*****************************************************************************/
bool CacheToolchainParser::validatePaths()
{
	if (!Commands::pathExists(m_state.toolchain.cpp()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C++ compiler was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(m_state.toolchain.cc()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's C compiler was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(m_state.toolchain.archiver()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's archive utility was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

	if (!Commands::pathExists(m_state.toolchain.linker()))
	{
#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
#endif
		Diagnostic::error(fmt::format("{}: The toolchain's linker was blank or could not be found.", m_jsonFile.filename()));
		return false;
	}

#if defined(CHALET_WIN32)
	if (!Commands::pathExists(m_state.toolchain.rc()))
	{
	#if defined(CHALET_DEBUG)
		m_jsonFile.dumpToTerminal();
	#endif
		Diagnostic::warn(fmt::format("{}: The toolchain's Windows Resource compiler was blank or could not be found.", m_jsonFile.filename()));
	}
#endif

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

	m_state.toolchain.detectToolchain();

	if (m_state.toolchain.detectedToolchain() == ToolchainType::LLVM)
	{
		if (m_inputs.targetArchitecture().empty())
		{
			// also takes -dumpmachine
			auto arch = Commands::subprocessOutput({ m_state.toolchain.compiler(), "-print-target-triple" });
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto isDarwin = arch.find("apple-darwin");
			if (isDarwin != std::string::npos)
			{
				arch = arch.substr(0, isDarwin + 12);
			}
#endif
			m_state.info.setTargetArchitecture(arch);
		}
	}
	else if (m_state.toolchain.detectedToolchain() == ToolchainType::GNU)
	{
		auto arch = Commands::subprocessOutput({ m_state.toolchain.compiler(), "-dumpmachine" });
		m_state.info.setTargetArchitecture(arch);
	}
#if defined(CHALET_WIN32)
	else if (m_state.toolchain.detectedToolchain() == ToolchainType::MSVC)
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
#if defined(CHALET_WIN32)
bool CacheToolchainParser::createMsvcEnvironment(const Json& inNode)
{
	bool readVariables = true;

	if (inNode.is_object())
	{
		auto keyEndsWith = [&inNode](const std::string& inKey, std::string_view inExt) {
			if (inNode.contains(inKey))
			{
				const Json& j = inNode[inKey];
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

	if (readVariables)
	{
		return m_state.msvcEnvironment.readCompilerVariables();
	}

	return true;
}
#endif

/*****************************************************************************/
bool CacheToolchainParser::makeToolchain(Json& toolchains, const ToolchainPreference& toolchain)
{
	bool result = true;

	if (!toolchains.contains(kKeyStrategy) || !toolchains[kKeyStrategy].is_string() || toolchains[kKeyStrategy].get<std::string>().empty())
	{
		// Note: this is only for validation. it gets changed later
		if (toolchain.strategy == StrategyType::Makefile)
		{
			toolchains[kKeyStrategy] = "makefile";
		}
		else if (toolchain.strategy == StrategyType::Ninja)
		{
			toolchains[kKeyStrategy] = "ninja";
		}
		else if (toolchain.strategy == StrategyType::Native)
		{
			toolchains[kKeyStrategy] = "native-experimental";
		}

		m_jsonFile.setDirty(true);
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

		parseArchitecture(cpp);
		result &= !cpp.empty();

		toolchains[kKeyCpp] = cpp;
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyCc))
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

		toolchains[kKeyCc] = cc;
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyLinker))
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

		parseArchitecture(link);
		result &= !link.empty();

		toolchains[kKeyLinker] = std::move(link);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyArchiver))
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

		toolchains[kKeyArchiver] = std::move(ar);
		m_jsonFile.setDirty(true);
	}

	if (!toolchains.contains(kKeyWindowsResource))
	{
		std::string rc;
		rc = Commands::which(toolchain.rc);

		parseArchitecture(rc);
		toolchains[kKeyWindowsResource] = std::move(rc);
		m_jsonFile.setDirty(true);
	}

	if (!result)
	{
		toolchains.erase(kKeyCpp);
		toolchains.erase(kKeyCc);
		toolchains.erase(kKeyLinker);
		toolchains.erase(kKeyArchiver);
		toolchains.erase(kKeyWindowsResource);
	}

	auto whichAdd = [&](Json& inNode, const std::string& inKey) -> bool {
		if (!inNode.contains(inKey))
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

	if (!toolchains.contains(kKeyMake))
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

		toolchains[kKeyMake] = std::move(make);
		m_jsonFile.setDirty(true);
	}

	whichAdd(toolchains, kKeyNinja);
	whichAdd(toolchains, kKeyObjdump);

	return result;
}

/*****************************************************************************/
bool CacheToolchainParser::parseToolchain(Json& inNode)
{
	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyStrategy))
		m_state.toolchain.setStrategy(val);

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyArchiver))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyArchiver] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setArchiver(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCpp))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyCpp] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setCpp(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCc))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyCc] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setCc(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyLinker))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyLinker] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setLinker(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyWindowsResource))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyWindowsResource] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setRc(std::move(val));
	}

	//

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyCmake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyCmake] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setCmake(std::move(val));
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyMake))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyMake] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setMake(std::move(val));
		m_make = std::move(val);
	}

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyNinja))
		m_state.toolchain.setNinja(std::move(val));

	if (std::string val; m_jsonFile.assignFromKey(val, inNode, kKeyObjdump))
	{
#if defined(CHALET_WIN32)
		if (!parseArchitecture(val))
		{
			inNode[kKeyObjdump] = val;
			m_jsonFile.setDirty(true);
		}
#endif
		m_state.toolchain.setObjdump(std::move(val));
	}

	return true;
}

/*****************************************************************************/
bool CacheToolchainParser::parseArchitecture(std::string& outString) const
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
