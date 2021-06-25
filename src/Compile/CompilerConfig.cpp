/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerConfig.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerConfig::CompilerConfig(const CodeLanguage inLanguage, const BuildState& inState) :
	kCompilerStructures({
		{ "/bin/hostx64/x64", "/lib/x64" },
		{ "/bin/hostx64/x86", "/lib/x86" },
		{ "/bin/hostx86/x86", "/lib/x86" },
		{ "/bin/hostx86/x64", "/lib/x64" },
		// { "/bin/hostx64/x64", "/lib/64" }, // TODO: Not sure what makes this different from /lib/x64
		{ "/bin", "/lib" },
	}),
	m_state(inState),
	m_language(inLanguage)
{
}

/*****************************************************************************/
CodeLanguage CompilerConfig::language() const noexcept
{
	return m_language;
}

/*****************************************************************************/
bool CompilerConfig::isInitialized() const noexcept
{
	return m_language != CodeLanguage::None;
}

/*****************************************************************************/
const std::string& CompilerConfig::compilerExecutable() const noexcept
{
	if (m_language == CodeLanguage::CPlusPlus)
		return m_state.toolchain.cpp();
	else
		return m_state.toolchain.cc();
}

/*****************************************************************************/
bool CompilerConfig::configureCompilerPaths()
{
	const auto& exec = compilerExecutable();
	chalet_assert(!exec.empty(), "No compiler was found");

	auto language = m_language == CodeLanguage::CPlusPlus ? "C++" : "C";
	if (exec.empty())
	{
		Diagnostic::error("Compiler executable was empty for language: '{}'", language);
		return false;
	}

	std::string path = String::getPathFolder(exec);
	const std::string lowercasePath = String::toLowerCase(path);

	for (const auto& [binDir, libDir] : kCompilerStructures)
	{
		if (String::endsWith(binDir, lowercasePath))
		{
			path = path.substr(0, path.size() - binDir.size());

#if defined(CHALET_MACOS)
			auto& xcodePath = Commands::getXcodePath();
			String::replaceAll(path, xcodePath, "");
			String::replaceAll(path, "/Toolchains/XcodeDefault.xctoolchain", "");
#endif
			m_compilerPathBin = path + binDir;
			m_compilerPathLib = path + libDir;
			m_compilerPathInclude = path + "/include";

			return true;
		}
	}

	Diagnostic::error("Invalid compiler structure (no 'bin' folder) found for language: '{}'", language);
	return false;
}

/*****************************************************************************/
bool CompilerConfig::testCompilerMacros()
{
	const auto& exec = compilerExecutable();
	if (exec.empty())
	{
		Diagnostic::error("No compiler executable was found");
		return false;
	}

#if defined(CHALET_WIN32)
	if (String::endsWith("/cl.exe", exec))
	{
		m_compilerType = CppCompilerType::VisualStudio;
		return true;
	}
#endif

	const std::string macroResult = Commands::testCompilerFlags(exec);
	// String::replaceAll(macroResult, '\n', ' ');
	// String::replaceAll(macroResult, "#include ", "");
	if (macroResult.empty())
	{
		Diagnostic::error("Failed to query predefined compiler macros.");
		return false;
	}

	// Notes:
	// GCC will just have __GNUC__
	// Clang will have both __clang__ & __GNUC__ (based on GCC 4)
	// Emscription will have __EMSCRIPTEN__, __clang__ & __GNUC__ (based on Clang)
	// Apple Clang (Xcode/CommandLineTools) is detected from __VERSION__ (for now),
	//   since one can install both GCC and Clang from Homebrew, which will also contain __APPLE__ & __APPLE_CC__
	// GCC in MinGW 32, MinGW-w64 32-bit will have both __GNUC__ and __MINGW32__
	// GCC in MinGW-w64 64-bit will also have __MINGW64__
	// Intel will have __INTEL_COMPILER (or at the very least __INTEL_COMPILER_BUILD_DATE) & __GNUC__ (Also GCC-based as far as I know)

	const bool clang = String::contains("__clang__", macroResult);
	const bool gcc = String::contains("__GNUC__", macroResult);
	const bool mingw32 = String::contains("__MINGW32__", macroResult);
	const bool mingw64 = String::contains("__MINGW64__", macroResult);
	const bool mingw = (mingw32 || mingw64);
	const bool emscripten = String::contains("__EMSCRIPTEN__", macroResult);
	const bool intel = String::contains({ "__INTEL_COMPILER", "__INTEL_COMPILER_BUILD_DATE" }, macroResult);
	const bool appleClang = clang && String::contains("Apple LLVM", macroResult);
	bool result = true;

	if (emscripten)
		m_compilerType = CppCompilerType::EmScripten;
	else if (appleClang)
		m_compilerType = CppCompilerType::AppleClang;
	else if (clang && mingw)
		m_compilerType = CppCompilerType::MingwClang;
	else if (clang)
		m_compilerType = CppCompilerType::Clang;
	else if (intel)
		m_compilerType = CppCompilerType::Intel;
	else if (gcc && mingw)
		m_compilerType = CppCompilerType::MingwGcc;
	else if (gcc)
		m_compilerType = CppCompilerType::Gcc;
	else
	{
		m_compilerType = CppCompilerType::Unknown;
		result = false;
	}

	// LOG(fmt::format("gcc: {}, clang: {}, appleClang: {}, mingw32: {}, mingw64: {}, emscripten: {}, intel: {},", gcc, clang, appleClang, mingw32, mingw64, emscripten, intel));
	// LOG("m_compilerType: ", static_cast<int>(m_compilerType));

	return result;
}

/*****************************************************************************/
bool CompilerConfig::getSupportedCompilerFlags()
{
	if (m_compilerType == CppCompilerType::Unknown)
		return false;

	const auto& exec = compilerExecutable();
	if (exec.empty())
		return false;

	if (m_state.info.targetArchitecture() == Arch::Cpu::X86)
		m_flagsFile = m_state.cache.getHashPath("flags_x86.env", CacheType::Local);
	else
		m_flagsFile = m_state.cache.getHashPath("flags_x64.env", CacheType::Local);

	m_state.cache.file().addExtraHash(String::getPathFilename(m_flagsFile));

	if (!Commands::pathExists(m_flagsFile))
	{
		if (isGcc())
		{
			{
				StringList helps{
					"common",
					"optimizers",
					//"params",
					"target",
					"warnings",
					"undocumented",
				};
				StringList cmd{ exec, "-Q" };
				for (auto& help : helps)
				{
					cmd.push_back(fmt::format("--help={}", help));
				}
				parseGnuHelpList(cmd);
			}
			{
				StringList cmd{ exec, "-Wl,--help" };
				parseGnuHelpList(cmd);
			}

			// TODO: separate/joined -- kind of weird to check for
		}
		else if (isClang())
		{
			parseClangHelpList();
		}

		std::string outContents;
		for (auto& [flag, _] : m_supportedFlags)
		{
			outContents += flag + "\n";
		}
		std::ofstream(m_flagsFile) << outContents;
	}
	else
	{
		std::ifstream input(m_flagsFile);
		for (std::string line; std::getline(input, line);)
		{
			m_supportedFlags[std::move(line)] = true;
		}
	}

	return true;
}

/*****************************************************************************/
void CompilerConfig::parseGnuHelpList(const StringList& inCommand)
{
	std::string raw = Commands::subprocessOutput(inCommand);
	auto split = String::split(raw, String::eol());

	for (auto& line : split)
	{
		if (String::contains("-fno-rtti", line))
		{
			LOG(line);
		}

		auto beg = line.find_first_not_of(' ');
		auto end = line.find_first_of('=', beg);
		if (end == std::string::npos)
		{
			end = line.find_first_of('<', beg);
			if (end == std::string::npos)
			{
				end = line.find_first_of(' ', beg);
			}
		}

		if (beg != std::string::npos && end != std::string::npos)
		{
			line = line.substr(beg, end - beg);
		}

		if (String::startsWith('-', line))
		{
			if (String::contains('\t', line))
			{
				auto afterTab = line.find_last_of('\t');
				if (afterTab != std::string::npos)
				{
					std::string secondFlag = line.substr(afterTab);

					if (String::startsWith('-', secondFlag))
					{
						if (m_supportedFlags.find(secondFlag) == m_supportedFlags.end())
							m_supportedFlags.emplace(String::toLowerCase(secondFlag), true);
					}
				}

				end = line.find_first_of('"');
				if (end == std::string::npos)
				{
					end = line.find_first_of(' ');
				}

				line = line.substr(beg, end - beg);

				if (String::startsWith('-', line))
				{
					if (m_supportedFlags.find(line) == m_supportedFlags.end())
						m_supportedFlags.emplace(String::toLowerCase(line), true);
				}
			}
			else
			{
				if (m_supportedFlags.find(line) == m_supportedFlags.end())
					m_supportedFlags.emplace(String::toLowerCase(line), true);
			}
		}
	}
}

/*****************************************************************************/
void CompilerConfig::parseClangHelpList()
{
	const auto& exec = compilerExecutable();

	std::string raw = Commands::subprocessOutput({ exec, "-cc1", "--help" });
	auto split = String::split(raw, String::eol());

	for (auto& line : split)
	{
		if (String::contains("-frtti", line))
		{
			LOG(line);
		}
		auto beg = line.find_first_not_of(' ');
		auto end = line.find_first_of('=', beg);
		if (end == std::string::npos)
		{
			end = line.find_first_of('<', beg);
			if (end == std::string::npos)
			{
				end = line.find_first_of(' ', beg);
			}
		}

		if (beg != std::string::npos && end != std::string::npos)
		{
			line = line.substr(beg, end - beg);
		}

		if (line.back() == ' ')
			line.pop_back();
		if (line.back() == ',')
			line.pop_back();

		if (String::startsWith('-', line))
		{
			if (String::contains('\t', line))
			{
				auto afterTab = line.find_last_of('\t');
				if (afterTab != std::string::npos)
				{
					std::string secondFlag = line.substr(afterTab);

					if (String::startsWith('-', secondFlag))
					{
						if (secondFlag.back() == ' ')
							secondFlag.pop_back();
						if (secondFlag.back() == ',')
							secondFlag.pop_back();

						if (m_supportedFlags.find(secondFlag) == m_supportedFlags.end())
							m_supportedFlags.emplace(String::toLowerCase(secondFlag), true);
					}
				}

				end = line.find_first_of('"');
				if (end == std::string::npos)
				{
					end = line.find_first_of(' ');
				}

				line = line.substr(beg, end - beg);

				if (String::startsWith('-', line))
				{
					if (line.back() == ' ')
						line.pop_back();
					if (line.back() == ',')
						line.pop_back();

					if (m_supportedFlags.find(line) == m_supportedFlags.end())
						m_supportedFlags.emplace(String::toLowerCase(line), true);
				}
			}
			else
			{
				if (m_supportedFlags.find(line) == m_supportedFlags.end())
					m_supportedFlags.emplace(String::toLowerCase(line), true);
			}
		}
	}

	// std::string supported;
	// for (auto& [flag, _] : m_supportedFlags)
	// {
	// 	supported += flag + '\n';
	// }
	// std::ofstream("clang_flags.txt") << supported;
}

/*****************************************************************************/
bool CompilerConfig::isFlagSupported(const std::string& inFlag) const
{
	return m_supportedFlags.find(inFlag) != m_supportedFlags.end();
}

/*****************************************************************************/
bool CompilerConfig::isLinkSupported(const std::string& inLink, const StringList& inDirectories) const
{
	const auto& exec = compilerExecutable();
	if (exec.empty())
		return false;

	if (isGcc())
	{
		// This will print the input if the link is not found in path
		auto file = fmt::format("lib{}.a", inLink);
		StringList cmd{ exec };
		for (auto& dir : inDirectories)
		{
			cmd.push_back(dir);
		}
		cmd.push_back(fmt::format("-print-file-name={}", file));

		auto raw = Commands::subprocessOutput(cmd);
		// auto split = String::split(raw, String::eol());

		return !String::equals(file, raw);
	}

	return true;
}

/*****************************************************************************/
CppCompilerType CompilerConfig::compilerType() const noexcept
{
	return m_compilerType;
}

bool CompilerConfig::isWindowsClang() const noexcept
{
#if defined(CHALET_WIN32)
	return m_compilerType == CppCompilerType::Clang;
#else
	return false;
#endif
}

bool CompilerConfig::isClang() const noexcept
{
	return m_compilerType == CppCompilerType::Clang || m_compilerType == CppCompilerType::AppleClang || m_compilerType == CppCompilerType::MingwClang || m_compilerType == CppCompilerType::EmScripten;
}

bool CompilerConfig::isAppleClang() const noexcept
{
	return m_compilerType == CppCompilerType::AppleClang;
}

bool CompilerConfig::isGcc() const noexcept
{
	return m_compilerType == CppCompilerType::Gcc || m_compilerType == CppCompilerType::MingwGcc || m_compilerType == CppCompilerType::Intel;
}

bool CompilerConfig::isMingw() const noexcept
{
	return m_compilerType == CppCompilerType::MingwGcc || m_compilerType == CppCompilerType::MingwClang;
}

bool CompilerConfig::isMingwGcc() const noexcept
{
	return m_compilerType == CppCompilerType::MingwGcc;
}

/*****************************************************************************/
bool CompilerConfig::isMsvc() const noexcept
{
	return m_compilerType == CppCompilerType::VisualStudio;
}

/*****************************************************************************/
bool CompilerConfig::isClangOrMsvc() const noexcept
{
	return isClang() || isMsvc();
}

/*****************************************************************************/
const std::string& CompilerConfig::compilerPathBin() const noexcept
{
	return m_compilerPathBin;
}
const std::string& CompilerConfig::compilerPathLib() const noexcept
{
	return m_compilerPathLib;
}
const std::string& CompilerConfig::compilerPathInclude() const noexcept
{
	return m_compilerPathInclude;
}

}
