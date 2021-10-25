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
		type = ICompileEnvironment::detectToolchainTypeFromPath(inState.toolchain.compilerCxx());
		if (type == ToolchainType::Unknown)
		{
			Diagnostic::error("Toolchain was not recognized from compiler path: '{}'", inState.toolchain.compilerCxx());
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
ToolchainType ICompileEnvironment::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
const std::string& ICompileEnvironment::detectedVersion() const
{
	return m_detectedVersion;
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

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	if (String::contains({ "icx", "oneAPI", "Intel", "oneapi", "intel" }, inExecutable))
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

		// TODO: ToolchainType::MingwGNU

		return ToolchainType::GNU;
	}

#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	#if defined(CHALET_WIN32)
	if (String::endsWith("/icl.exe", inExecutable))
	#else
	if (String::endsWith({ "/icpc", "/icc" }, inExecutable))
	#endif
		return ToolchainType::IntelClassic;
#endif

	return ToolchainType::Unknown;
}

}
