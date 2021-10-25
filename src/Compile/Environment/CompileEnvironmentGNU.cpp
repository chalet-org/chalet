/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentGNU::CompileEnvironmentGNU(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inType, inInputs, inState)
{
}

/*****************************************************************************/
StringList CompileEnvironmentGNU::getVersionCommand(const std::string& inExecutable) const
{
	return { inExecutable, "-v" };
}

/*****************************************************************************/
std::string CompileEnvironmentGNU::getFullCxxCompilerString(const std::string& inVersion) const
{
	return fmt::format("GNU Compiler Collection C/C++ version {}", inVersion);
}

/*****************************************************************************/
CompilerInfo CompileEnvironmentGNU::getCompilerInfoFromExecutable(const std::string& inExecutable) const
{
	CompilerInfo ret;

	auto& sourceCache = m_state.cache.file().sources();
	std::string cachedVersion;
	if (sourceCache.versionRequriesUpdate(inExecutable, cachedVersion))
	{
		// Expects:
		// gcc version 10.2.0 (Ubuntu 10.2.0-13ubuntu1)
		// gcc version 10.2.0 (Rev10, Built by MSYS2 project)
		// Apple clang version 12.0.5 (clang-1205.0.22.9)

		const auto exec = String::getPathBaseName(inExecutable);
		std::string rawOutput = Commands::subprocessOutput(getVersionCommand(inExecutable));

		StringList splitOutput;
#if defined(CHALET_WIN32)
		if (rawOutput.find('\r') != std::string::npos)
			splitOutput = String::split(rawOutput, "\r\n");
		else
			splitOutput = String::split(rawOutput, '\n');
#else
		splitOutput = String::split(rawOutput, '\n');
#endif

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

			version = version.substr(0, version.find_first_not_of("0123456789."));
			if (!version.empty())
				cachedVersion = std::move(version);

			// if (!arch.empty())
			// 	ret.arch = std::move(arch);
			// else
			// ret.arch = m_state.info.targetArchitectureTriple();
		}
	}

	if (!cachedVersion.empty())
	{
		ret.version = std::move(cachedVersion);

		sourceCache.addVersion(inExecutable, ret.version);

		ret.arch = m_state.info.targetArchitectureTriple();
		ret.description = getFullCxxCompilerString(ret.version);
	}
	else
	{
		ret.description = "Unrecognized";
	}

	return ret;
}

/*****************************************************************************/
bool CompileEnvironmentGNU::verifyToolchain()
{
	const auto& compiler = m_state.toolchain.compilerCxx();
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
bool CompileEnvironmentGNU::verifyCompilerExecutable(const std::string& inCompilerExec)
{
	const std::string macroResult = getCompilerMacros(inCompilerExec);
	// LOG(macroResult);
	// LOG(exec);
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

	ToolchainType detectedType = getToolchainTypeFromMacros(macroResult);
	return detectedType == m_type;
}

/*****************************************************************************/
void CompileEnvironmentGNU::parseVersionFromVersionOutput(const std::string& inLine, std::string& outVersion) const
{
	auto start = inLine.find("version");
	if (start == std::string::npos)
		return;

	outVersion = inLine.substr(start + 8);

	while (outVersion.back() == ' ')
		outVersion.pop_back();
}

/*****************************************************************************/
void CompileEnvironmentGNU::parseArchFromVersionOutput(const std::string& inLine, std::string& outArch) const
{
	if (!String::startsWith("Target:", inLine))
		return;

	outArch = inLine.substr(8);
}

/*****************************************************************************/
void CompileEnvironmentGNU::parseThreadModelFromVersionOutput(const std::string& inLine, std::string& outThreadModel) const
{
	if (!String::startsWith("Thread model:", inLine))
		return;

	outThreadModel = inLine.substr(14);
}

/*****************************************************************************/
bool CompileEnvironmentGNU::makeArchitectureAdjustments()
{
	const auto& archTriple = m_state.info.targetArchitectureTriple();
	const auto& compiler = m_state.toolchain.compilerCxx();

	if (m_inputs.targetArchitecture().empty() || !String::contains('-', archTriple))
	{
		auto& sourceCache = m_state.cache.file().sources();
		std::string cachedArch;
		if (sourceCache.archRequriesUpdate(compiler, cachedArch))
		{
			cachedArch = Commands::subprocessOutput({ compiler, "-dumpmachine" });
#if defined(CHALET_MACOS)
			// Strip out version in auto-detected mac triple
			auto darwin = cachedArch.find("apple-darwin");
			if (darwin != std::string::npos)
			{
				cachedArch = cachedArch.substr(0, darwin + 12);
			}
#endif
		}

		m_state.info.setTargetArchitecture(cachedArch);
		sourceCache.addArch(compiler, cachedArch);
	}
	else
	{
		// Pass along and hope for the best

		// if (!String::startsWith(archFromInput, archTriple))
		// 	return false;
	}

	return true;
}

/*****************************************************************************/
bool CompileEnvironmentGNU::validateArchitectureFromInput()
{
	if (String::equals("gcc", m_inputs.toolchainPreferenceName()))
	{
		const auto& arch = m_state.info.targetArchitectureString();
		m_inputs.setToolchainPreferenceName(fmt::format("{}-gcc", arch));
	}

	return true;
}

/*****************************************************************************/
ToolchainType CompileEnvironmentGNU::getToolchainTypeFromMacros(const std::string& inMacros) const
{
	const bool gcc = String::contains("__GNUC__", inMacros);
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	// const bool mingw32 = String::contains("__MINGW32__", inMacros);
	// const bool mingw64 = String::contains("__MINGW64__", inMacros);
	// const bool mingw = (mingw32 || mingw64);

	// if (gcc && mingw)
	// 	return ToolchainType::MingwGNU;
	// else
#endif
	if (gcc)
		return ToolchainType::GNU;

	return ToolchainType::Unknown;
}

/*****************************************************************************/
std::string CompileEnvironmentGNU::getCompilerMacros(const std::string& inCompilerExec)
{
	if (inCompilerExec.empty())
		return std::string();

	std::string macrosFile = m_state.cache.getHashPath(fmt::format("macros_{}.env", inCompilerExec), CacheType::Local);
	m_state.cache.file().addExtraHash(String::getPathFilename(macrosFile));

	std::string result;
	if (!Commands::pathExists(macrosFile))
	{
#if defined(CHALET_WIN32)
		std::string null = "nul";
#else
		std::string null = "/dev/null";
#endif

		// Clang/GCC only
		// This command must be run from the bin directory in order to work
		//   (or added to path before-hand, but we manipulate the path later)
		//
		auto compilerPath = String::getPathFolder(inCompilerExec);
		StringList command = { inCompilerExec, "-x", "c", std::move(null), "-dM", "-E" };
		result = Commands::subprocessOutput(command, std::move(compilerPath));

		std::ofstream(macrosFile) << result;
	}
	else
	{
		std::ifstream input(macrosFile);
		result = std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
	}

	return result;
}
}