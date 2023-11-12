/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "BuildEnvironment/BuildEnvironmentGNU.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
BuildEnvironmentGNU::BuildEnvironmentGNU(const ToolchainType inType, BuildState& inState) :
	IBuildEnvironment(inType, inState)
{
}

/*****************************************************************************/
StringList BuildEnvironmentGNU::getVersionCommand(const std::string& inExecutable) const
{
	return StringList{ inExecutable, "-v" };
}

/*****************************************************************************/
std::string BuildEnvironmentGNU::getCompilerFlavor(const std::string& inPath) const
{
	// Get the MSYSTEM from the executable path (expects MSYSTEM/bin)
	//
	auto flavor = String::getPathFilename(String::getPathFolder(String::getPathFolder(inPath)));
	if (!flavor.empty() && (String::endsWith("64", flavor) || String::endsWith("32", flavor)))
	{
		flavor = String::toUpperCase(flavor);
		flavor = fmt::format(" ({})", flavor);
	}
	else
	{
		flavor.clear();
	}
	return flavor;
}

/*****************************************************************************/
std::string BuildEnvironmentGNU::getFullCxxCompilerString(const std::string& inPath, const std::string& inVersion) const
{
	if (m_state.toolchain.treatAs() == CustomToolchainTreatAs::GCC)
	{
		auto name = String::getPathBaseName(inPath);
		if (!name.empty())
		{
			String::capitalize(name);
			String::replaceAll(name, '+', "");
		}

		return fmt::format("{} version {} (Based on GCC)", name, inVersion);
	}
	else if (m_type == ToolchainType::MingwGNU)
	{
		auto flavor = getCompilerFlavor(inPath);
		return fmt::format("Minimalist GNU Compiler Collection for Windows version {}{}", inVersion, flavor);
	}
	else
	{
		return fmt::format("GNU Compiler Collection version {}", inVersion);
	}
}

/*****************************************************************************/
bool BuildEnvironmentGNU::getCompilerVersionAndDescription(CompilerInfo& outInfo) const
{
	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedVersion;
	if (sourceCache.versionRequriesUpdate(outInfo.path, cachedVersion))
	{
		// Expects:
		// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
		// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
		// Apple clang version 12.0.5 (clang-1205.0.22.9)

		const auto exec = String::getPathBaseName(outInfo.path);
		std::string rawOutput = Files::subprocessOutput(getVersionCommand(outInfo.path));

		StringList splitOutput;
		splitOutput = String::split(rawOutput, '\n');

		if (splitOutput.size() >= 2)
		{
			std::string version;
			// std::string threadModel;
			// std::string arch;
			for (auto& line : splitOutput)
			{
				parseVersionFromVersionOutput(line, version);
				// parseArchFromVersionOutput(line, arch);
				// parseThreadModelFromVersionOutput(line, threadModel);
			}

#if defined(CHALET_LINUX)
			if (Shell::isWindowsSubsystemForLinux())
			{
				if (String::contains({ "(GCC)", "-win32 " }, version))
					version = version.substr(0, version.find(" ("));
				else
					version = version.substr(0, version.find_first_not_of("0123456789."));
			}
			else
#endif
			{
				version = version.substr(0, version.find_first_not_of("0123456789."));
			}

			if (!version.empty())
				cachedVersion = std::move(version);
		}
	}

	if (!cachedVersion.empty())
	{
		outInfo.version = std::move(cachedVersion);

		sourceCache.addVersion(outInfo.path, outInfo.version);

		outInfo.description = getFullCxxCompilerString(outInfo.path, outInfo.version);
		return true;
	}
	else
	{
		outInfo.description = "Unrecognized";
		return false;
	}
}

/*****************************************************************************/
std::vector<CompilerPathStructure> BuildEnvironmentGNU::getValidCompilerPaths() const
{
	std::vector<CompilerPathStructure> ret;
	auto triple = m_state.info.targetArchitectureTriple();
	ret.push_back({ "/bin", fmt::format("/{}/lib", triple), fmt::format("/{}/include", triple) });
#if defined(CHALET_LINUX)
	ret.push_back({ "/bin", fmt::format("/lib/{}/lib", triple), fmt::format("/lib/{}/include", triple) });
#endif
	ret.push_back({ "/bin", "/lib", "/include" });
	return ret;
}

/*****************************************************************************/
bool BuildEnvironmentGNU::populateSupportedFlags(const std::string& inExecutable)
{
	{
		StringList categories{
			"common",
			"optimizers",
			//"params",
			"target",
			"warnings",
			"undocumented",
		};
		StringList cmd{ inExecutable, "-Q" };
		for (auto& category : categories)
		{
			cmd.emplace_back(fmt::format("--help={}", category));
		}
		parseSupportedFlagsFromHelpList(cmd);
	}
	{
		StringList cmd{ inExecutable, "-Wl,--help" };
		parseSupportedFlagsFromHelpList(cmd);
	}

	// TODO: separate/joined -- kind of weird to check for

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentGNU::verifyToolchain()
{
	const auto& compiler = m_state.toolchain.compilerCxxAny().path;
	if (compiler.empty())
	{
		Diagnostic::error("No compiler executable was found");
		return false;
	}

	if (!verifyCompilerExecutable(compiler))
		return false;

	// if (!verifyCompilerExecutable(m_state.toolchain.compilerC()))
	// 	return false;

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentGNU::supportsFlagFile()
{
	return true;
}

/*****************************************************************************/
bool BuildEnvironmentGNU::verifyCompilerExecutable(const std::string& inCompilerExec)
{
	const std::string macroResult = getCompilerMacros(inCompilerExec, m_state);
	// LOG(macroResult);
	// LOG(inCompilerExec);
	// String::replaceAll(macroResult, '\n', ' ');
	// String::replaceAll(macroResult, "#include ", "");
	if (macroResult.empty())
	{
		auto output = getCompilerMacros(inCompilerExec, m_state, PipeOption::Pipe);
		Output::print(Color::Reset, output);
		Diagnostic::error("Failed to query compiler for details. See above output.");
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

	ToolchainType detectedType = getToolchainTypeFromMacros(macroResult);
	// LOG("types:", static_cast<i32>(detectedType), static_cast<i32>(m_type));
	if (detectedType != m_type)
	{
		Diagnostic::error("No compiler executable was found");
		return false;
	}

	return true;
}

/*****************************************************************************/
void BuildEnvironmentGNU::parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const
{
	auto start = inLine.find("version");
	if (start == std::string::npos)
		return;

	outVersion = inLine.substr(start + 8);

	while (outVersion.back() == ' ')
		outVersion.pop_back();
}

/*****************************************************************************/
void BuildEnvironmentGNU::parseArchFromVersionOutput(const std::string& inLine, std::string& outArch) const
{
	if (!String::startsWith("Target:", inLine))
		return;

	outArch = inLine.substr(8);
}

/*****************************************************************************/
void BuildEnvironmentGNU::parseThreadModelFromVersionOutput(const std::string& inLine, std::string& outThreadModel) const
{
	if (!String::startsWith("Thread model:", inLine))
		return;

	outThreadModel = inLine.substr(14);
}

/*****************************************************************************/
bool BuildEnvironmentGNU::readArchitectureTripleFromCompiler()
{
	const auto& targetTriple = m_state.info.targetArchitectureTriple();
	const auto& compiler = m_state.toolchain.compilerCxxAny().path;

	bool emptyInputArch = m_state.inputs.targetArchitecture().empty();
	if (emptyInputArch || !String::contains('-', targetTriple))
	{
		auto& sourceCache = m_state.cache.file().sources();
		std::string cachedArch;
		if (sourceCache.archRequriesUpdate(compiler, cachedArch))
		{
			cachedArch = Files::subprocessOutput({ compiler, "-dumpmachine" });

			// Make our corrections here
			//
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto darwin = cachedArch.find("apple-darwin");
			if (darwin != std::string::npos)
			{
				cachedArch = cachedArch.substr(0, darwin + 12);
			}
#else
			// Note: Standalone "mingw32" is used in 32-bit TDM GCC MinGW builds for some reason
			if (String::equals("mingw32", cachedArch))
			{
				cachedArch = "i686-pc-mingw32";
			}
#endif
		}

		std::string archFromInfo = m_state.info.targetArchitectureString();
		if (String::equals("armhf", archFromInfo))
		{
			archFromInfo = "arm";
		}

		if (!emptyInputArch && !String::startsWith(archFromInfo, cachedArch))
		{
			auto expectedArch = Arch::from(cachedArch);
			Diagnostic::error("Expected '{}' or '{}'. Please use a different toolchain or create a new one for this architecture.", cachedArch, expectedArch.str);
			/*if (m_genericGcc)
			{
				// const auto& arch = m_state.info.targetArchitectureString();
				// auto name = m_state.inputs.toolchainPreferenceName();
				// String::replaceAll(name, arch, expectedArch.str);
				// m_state.inputs.setToolchainPreferenceName(std::move(name));
			}*/
			return false;
		}

		m_state.info.setTargetArchitecture(cachedArch);
		sourceCache.addArch(compiler, cachedArch);
	}

	m_isWindowsTarget = String::contains(StringList{ "windows", "win32", "msvc", "mingw32", "w64" }, m_state.info.targetArchitectureTriple());
	m_isEmbeddedTarget = String::contains(StringList{ "-none-eabi" }, m_state.info.targetArchitectureTriple());

	return true;
}

/*****************************************************************************/
bool BuildEnvironmentGNU::validateArchitectureFromInput()
{
	/*auto& toolchain = m_state.inputs.toolchainPreferenceName();
	// If the tooclhain was a preset and was not a target triple
	if (m_state.inputs.isToolchainPreset() && (String::equals("gcc", toolchain) || String::startsWith("gcc-", toolchain)))
	{
		// const auto& arch = m_state.info.targetArchitectureString();
		// m_state.inputs.setToolchainPreferenceName(fmt::format("{}-{}", arch, toolchain));
		m_genericGcc = true;
	}*/

	return true;
}

/*****************************************************************************/
ToolchainType BuildEnvironmentGNU::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	const bool gcc = String::contains("__GNUC__", inMacros);
	const bool mingw32 = String::contains("__MINGW32__", inMacros);
	const bool mingw64 = String::contains("__MINGW64__", inMacros);
	const bool mingw = (mingw32 || mingw64);

	if (gcc && mingw)
		return ToolchainType::MingwGNU;
	else if (gcc)
		return ToolchainType::GNU;

	return ToolchainType::Unknown;
}

/*****************************************************************************/
std::string BuildEnvironmentGNU::getCompilerMacros(const std::string& inCompilerExec, BuildState& inState, const PipeOption inStdError)
{
	if (inCompilerExec.empty())
		return std::string();

	std::string macrosFile = inState.cache.getHashPath(fmt::format("macros_{}.env", inCompilerExec), CacheType::Local);
	inState.cache.file().addExtraHash(String::getPathFilename(macrosFile));

	std::string result;
	if (!Files::pathExists(macrosFile))
	{
		// Clang/GCC only
		// This command must be run from the bin directory in order to work
		//   (or added to path before-hand, but we manipulate the path later)
		//
		auto compilerPath = String::getPathFolder(inCompilerExec);
		StringList command = { inCompilerExec, "-x", "c", Shell::getNull(), "-dM", "-E" };
		result = Files::subprocessOutput(command, std::move(compilerPath), PipeOption::Pipe, inStdError);

		if (!result.empty())
		{
			std::ofstream(macrosFile) << result;
		}
	}
	else
	{
		std::ifstream input(macrosFile);
		result = std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	}

	return result;
}

/*****************************************************************************/
void BuildEnvironmentGNU::parseSupportedFlagsFromHelpList(const StringList& inCommand)
{
	auto path = String::getPathFolder(inCommand.front());
	std::string raw = Files::subprocessOutput(inCommand, std::move(path));
	auto split = String::split(raw, '\n');

	for (auto& line : split)
	{
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
/*
	Resolve the system include directories. When cross-compiling, we have to
	explicitly use these with clang later.

	They are typically:
		/usr/(arch-triple)/ - libraries for this architecture
		/usr/lib/gcc/(arch-triple)/(version) - system libs only

	This is the system path include order (if they exist):
		/usr/lib/gcc/(arch-triple)/(version)/include/c++
		/usr/lib/gcc/(arch-triple)/(version)/include/c++/(arch-triple)
		/usr/lib/gcc/(arch-triple)/(version)/include/c++/backward
		/usr/lib/gcc/(arch-triple)/(version)/include
		/usr/lib/gcc/(arch-triple)/(version)/include-fixed
		/usr/(arch-triple)/include

	Viewed with:
		x86_64-w64-mingw32-gcc -xc++ -E -v -
*/
void BuildEnvironmentGNU::generateTargetSystemPaths()
{
#if defined(CHALET_LINUX)
	const auto& targetArch = m_state.info.targetArchitectureTriple();

	m_sysroot.clear();
	m_targetSystemVersion.clear();
	m_targetSystemPaths.clear();

	// TODO: if using a custom llvm & gcc toolchain build, user would need to give the path here
	auto basePath = "/usr";

	auto otherCompiler = fmt::format("{}/bin/{}-gcc", basePath, targetArch);
	if (Files::pathExists(otherCompiler))
	{
		auto version = Files::subprocessOutput({ otherCompiler, "-dumpfullversion" });
		if (!version.empty())
		{
			version = version.substr(0, version.find_first_not_of("0123456789."));
			if (!version.empty())
			{
				auto shortVersion = version.substr(0, version.find_first_not_of("0123456789"));

				auto sysroot = fmt::format("{}/{}", basePath, targetArch);
				if (!Files::pathExists(sysroot))
				{
					sysroot = fmt::format("{}/lib/{}", basePath, targetArch);
					if (!Files::pathExists(sysroot))
						sysroot.clear();
				}

				if (!sysroot.empty())
				{
					auto sysroot2 = fmt::format("{}/lib/gcc/{}/{}", basePath, targetArch, version);
					if (!Files::pathExists(sysroot2))
					{
						// TODO: way to control '-posix' or '-win32'
						//
						sysroot2 = fmt::format("{}/lib/gcc/{}/{}-posix", basePath, targetArch, version);
						if (!Files::pathExists(sysroot2))
						{
							sysroot2 = fmt::format("{}/lib/gcc-cross/{}/{}", basePath, targetArch, shortVersion);
							if (!Files::pathExists(sysroot2))
								sysroot2.clear();
						}
					}

					if (!sysroot2.empty())
					{
						// LOG(sysroot);
						// LOG(sysroot2);

						auto addInclude = [this](std::string&& path) {
							if (Files::pathExists(path))
								m_targetSystemPaths.emplace_back(std::move(path));
						};

						// Note: Do not change this order
						//
						if (!String::equals(shortVersion, version))
						{
							addInclude(fmt::format("{}/include/c++/{}", sysroot, shortVersion));
							addInclude(fmt::format("{}/include/c++/{}/{}", sysroot, shortVersion, targetArch));
							addInclude(fmt::format("{}/include/c++/{}/backward", sysroot, shortVersion));

							addInclude(fmt::format("{}/include/c++/{}", sysroot, version));
							addInclude(fmt::format("{}/include/c++/{}/{}", sysroot, version, targetArch));
							addInclude(fmt::format("{}/include/c++/{}/backward", sysroot, version));
						}
						else
						{
							addInclude(fmt::format("{}/include/c++/{}", sysroot, version));
							addInclude(fmt::format("{}/include/c++/{}/{}", sysroot, version, targetArch));
							addInclude(fmt::format("{}/include/c++/{}/backward", sysroot, version));
						}

						addInclude(fmt::format("{}/include/c++", sysroot2));
						addInclude(fmt::format("{}/include/c++/{}", sysroot2, targetArch));
						addInclude(fmt::format("{}/include/c++/backward", sysroot2));

						addInclude(fmt::format("{}/include", sysroot2));
						addInclude(fmt::format("{}/include-fixed", sysroot2));

						addInclude(fmt::format("{}/include", sysroot));
						addInclude(fmt::format("{}/include", basePath));

						// for (auto& path : m_targetSystemPaths)
						// {
						// 	LOG(path);
						// }

						// m_sysroot = sysroot;
						m_sysroot = sysroot2;
						m_targetSystemVersion = version;
					}
				}
			}
		}
	}
#endif
}
}