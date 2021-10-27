/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/ICompileEnvironment.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/DefinesExperimental.hpp"
#include "Utility/String.hpp"

#include "Compile/Environment/CompileEnvironmentAppleLLVM.hpp"
#include "Compile/Environment/CompileEnvironmentGNU.hpp"
#include "Compile/Environment/CompileEnvironmentIntel.hpp"
#include "Compile/Environment/CompileEnvironmentLLVM.hpp"
#include "Compile/Environment/CompileEnvironmentVisualStudio.hpp"

namespace chalet
{
/*****************************************************************************/
ToolchainType ICompileEnvironment::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool ICompileEnvironment::isWindowsClang() const noexcept
{
#if defined(CHALET_WIN32)
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::IntelLLVM;
#else
	return false;
#endif
}

/*****************************************************************************/
bool ICompileEnvironment::isClang() const noexcept
{
	return m_type == ToolchainType::LLVM
		|| m_type == ToolchainType::AppleLLVM
		|| m_type == ToolchainType::IntelLLVM
		|| m_type == ToolchainType::MingwLLVM
		|| m_type == ToolchainType::EmScripten;
}

/*****************************************************************************/
bool ICompileEnvironment::isAppleClang() const noexcept
{
	return m_type == ToolchainType::AppleLLVM;
}

/*****************************************************************************/
bool ICompileEnvironment::isGcc() const noexcept
{
	return m_type == ToolchainType::GNU
		|| m_type == ToolchainType::MingwGNU
		|| m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool ICompileEnvironment::isIntelClassic() const noexcept
{
	return m_type == ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool ICompileEnvironment::isMingw() const noexcept
{
	return m_type == ToolchainType::MingwGNU
		|| m_type == ToolchainType::MingwLLVM;
}

/*****************************************************************************/
bool ICompileEnvironment::isMingwGcc() const noexcept
{
	return m_type == ToolchainType::MingwGNU;
}

/*****************************************************************************/
bool ICompileEnvironment::isMsvc() const noexcept
{
	return m_type == ToolchainType::VisualStudio;
}

/*****************************************************************************/
bool ICompileEnvironment::isClangOrMsvc() const noexcept
{
	return isClang() || isMsvc();
}

/*****************************************************************************/
const std::string& ICompileEnvironment::detectedVersion() const
{
	return m_detectedVersion;
}

/*****************************************************************************/
bool ICompileEnvironment::isCompilerFlagSupported(const std::string& inFlag) const
{
	return m_supportedFlags.find(inFlag) != m_supportedFlags.end();
}

/*****************************************************************************/
// Protected/Private Town
//
ICompileEnvironment::ICompileEnvironment(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState),
	m_type(inType)
{
	UNUSED(m_inputs, m_state);
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompileEnvironment> ICompileEnvironment::make(ToolchainType type, const CommandLineInputs& inInputs, BuildState& inState)
{
	if (type == ToolchainType::Unknown)
	{
		auto& compiler = inState.toolchain.compilerCxxAny().path;
		type = ICompileEnvironment::detectToolchainTypeFromPath(compiler);
		if (type == ToolchainType::Unknown)
		{
			Diagnostic::error("Toolchain was not recognized from compiler path: '{}'", compiler);
			return nullptr;
		}
	}

	switch (type)
	{
		case ToolchainType::VisualStudio:
			return std::make_unique<CompileEnvironmentVisualStudio>(type, inInputs, inState);
		case ToolchainType::AppleLLVM:
			return std::make_unique<CompileEnvironmentAppleLLVM>(type, inInputs, inState);
		case ToolchainType::LLVM:
		case ToolchainType::MingwLLVM:
			return std::make_unique<CompileEnvironmentLLVM>(type, inInputs, inState);
		case ToolchainType::IntelClassic:
		case ToolchainType::IntelLLVM:
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC || CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
			return std::make_unique<CompileEnvironmentIntel>(type, inInputs, inState);
#endif
		case ToolchainType::GNU:
		case ToolchainType::MingwGNU:
			return std::make_unique<CompileEnvironmentGNU>(type, inInputs, inState);
		case ToolchainType::Unknown:
		default:
			break;
	}

	Diagnostic::error("Unimplemented ToolchainType requested: ", static_cast<int>(type));
	return nullptr;
}

/*****************************************************************************/
bool ICompileEnvironment::create(const std::string& inVersion)
{
	if (m_initialized)
	{
		Diagnostic::error("Compiler environment was already initialized.");
		return false;
	}

	m_initialized = true;

	if (!validateArchitectureFromInput())
		return false;

	if (!createFromVersion(inVersion))
		return false;

	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::getCompilerInfoFromExecutable(CompilerInfo& outInfo)
{
	if (outInfo.path.empty())
	{
		Diagnostic::error("Compiler executable was unexpectedly blank: '{}'", outInfo.path);
		return false;
	}

	if (!getCompilerPaths(outInfo))
	{
		Diagnostic::error("Unexpected compiler toolchain structure found from executable: '{}'", outInfo.path);
		return false;
	}

	if (!getCompilerVersionAndDescription(outInfo))
	{
		Diagnostic::error("Error getting the version and description for: '{}'", outInfo.path);
		return false;
	}

	if (!makeSupportedCompilerFlags(outInfo.path))
	{
		Diagnostic::error("Error collecting supported compiler flags.");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::makeSupportedCompilerFlags(const std::string& inExecutable)
{
	std::string flagsFile = m_state.cache.getHashPath(fmt::format("flags_{}.env", inExecutable), CacheType::Local);

	if (!Commands::pathExists(flagsFile))
	{
		if (populateSupportedFlags(inExecutable))
		{
			std::string outContents;
			for (auto& [flag, _] : m_supportedFlags)
			{
				outContents += flag + "\n";
			}

			std::ofstream(flagsFile) << outContents;

			m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
		}
	}
	else
	{
		std::ifstream input(flagsFile);
		for (std::string line; std::getline(input, line);)
		{
			m_supportedFlags[std::move(line)] = true;
		}

		m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));
	}

	return true;
}

bool ICompileEnvironment::populateSupportedFlags(const std::string& inExecutable)
{
	UNUSED(inExecutable);
	return true;
}

bool ICompileEnvironment::getCompilerPaths(CompilerInfo& outInfo) const
{
	std::string path = String::getPathFolder(outInfo.path);
	const std::string lowercasePath = String::toLowerCase(path);

	std::vector<CompilerPathStructure> compilerStructures = getValidCompilerPaths();

	for (const auto& [binDir, libDir, includeDir] : compilerStructures)
	{
		if (!String::endsWith(binDir, lowercasePath))
			continue;

		path = path.substr(0, path.size() - binDir.size());

#if defined(CHALET_MACOS)
		const auto& xcodePath = Commands::getXcodePath();
		String::replaceAll(path, xcodePath, "");
		String::replaceAll(path, "/Toolchains/XcodeDefault.xctoolchain", "");
#endif
		outInfo.binDir = path + binDir;
		outInfo.libDir = path + libDir;
		outInfo.includeDir = path + includeDir;

		return true;
	}

	return false;
}

/*****************************************************************************/
bool ICompileEnvironment::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::validateArchitectureFromInput()
{
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::makeArchitectureAdjustments()
{
	return true;
}

/*****************************************************************************/
bool ICompileEnvironment::compilerVersionIsToolchainVersion() const
{
	return true;
}

/*****************************************************************************/
std::string ICompileEnvironment::getVarsPath(const std::string& inId) const
{
	auto archString = m_inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureTriple());
	archString += fmt::format("_{}", m_inputs.toolchainPreferenceName());
	return m_state.cache.getHashPath(fmt::format("{}_{}.env", inId, archString), CacheType::Local);
}

/*****************************************************************************/
bool ICompileEnvironment::saveOriginalEnvironment(const std::string& inOutputFile) const
{
#if defined(CHALET_WIN32)
	auto cmdExe = Environment::getComSpec();
	StringList cmd{
		std::move(cmdExe),
		"/c",
		"SET"
	};
#else
	chalet_assert(m_state.tools.bashAvailable(), "");

	StringList cmd;
	cmd.push_back(m_state.tools.bash());
	cmd.emplace_back("-c");
	cmd.emplace_back("printenv");
#endif
	bool result = Commands::subprocessOutputToFile(cmd, inOutputFile);
	return result;
}

/*****************************************************************************/
void ICompileEnvironment::createEnvironmentDelta(const std::string& inOriginalFile, const std::string& inCompilerFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine) const
{
	if (inOriginalFile.empty() || inCompilerFile.empty() || inDeltaFile.empty())
		return;

	{
		std::ifstream compilerVarsInput(inCompilerFile);
		std::string compilerVars((std::istreambuf_iterator<char>(compilerVarsInput)), std::istreambuf_iterator<char>());

		std::ifstream originalVars(inOriginalFile);
		for (std::string line; std::getline(originalVars, line);)
		{
			String::replaceAll(compilerVars, line, "");
		}

		std::ofstream(inDeltaFile) << compilerVars;

		compilerVarsInput.close();
		originalVars.close();
	}

	Commands::remove(inOriginalFile);
	Commands::remove(inCompilerFile);

	{
		std::string outContents;
		std::ifstream input(inDeltaFile);
		for (std::string line; std::getline(input, line);)
		{
			if (!line.empty())
			{
				onReadLine(line);
				outContents += line + "\n";
			}
		}
		input.close();

		std::ofstream(inDeltaFile) << outContents;
	}
}

/*****************************************************************************/
void ICompileEnvironment::cacheEnvironmentDelta(const std::string& inDeltaFile)
{
	m_variables.clear();

	std::ifstream input(inDeltaFile);
	for (std::string line; std::getline(input, line);)
	{
		auto splitVar = String::split(line, '=');
		if (splitVar.size() == 2 && splitVar.front().size() > 0 && splitVar.back().size() > 0)
		{
			m_variables[std::move(splitVar.front())] = splitVar.back();
		}
	}
	input.close();
}

/*****************************************************************************/
ToolchainType ICompileEnvironment::detectToolchainTypeFromPath(const std::string& inExecutable)
{
	if (inExecutable.empty())
		return ToolchainType::Unknown;

#if defined(CHALET_WIN32)
	if (String::endsWith("/cl.exe", inExecutable))
		return ToolchainType::VisualStudio;
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	if (String::endsWith("/icl.exe", inExecutable))
	#else
	if (String::endsWith({ "/icpc", "/icc" }, inExecutable))
	#endif
		return ToolchainType::IntelClassic;
#endif

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	if (String::endsWith("/icx", inExecutable) || String::contains({ "oneAPI", "Intel", "oneapi", "intel" }, inExecutable))
		return ToolchainType::IntelLLVM;
#endif

	if (String::contains("clang", inExecutable))
	{
#if defined(CHALET_MACOS)
		if (String::contains({ "Contents/Developer", "Xcode" }, inExecutable))
			return ToolchainType::AppleLLVM;
#endif

		// TODO: ToolchainType::MingwLLVM

		return ToolchainType::LLVM;
	}

	if (String::contains({ "gcc", "g++" }, inExecutable))
	{
#if defined(CHALET_MACOS)
		if (String::contains({ "Contents/Developer", "Xcode" }, inExecutable))
			return ToolchainType::AppleLLVM;
#endif
#if defined(CHALET_WIN32)
		return ToolchainType::MingwGNU; // TODO: better way of checking - this is a bit of a hack
#else
		return ToolchainType::GNU;
#endif
	}

	return ToolchainType::Unknown;
}

}
