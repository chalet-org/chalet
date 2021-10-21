/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironment.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/DefinesExperimental.hpp"
#include "Utility/String.hpp"

#include "Compile/Environment/IntelCompileEnvironment.hpp"
#include "Compile/Environment/VisualStudioCompileEnvironment.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironment::CompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState)
{
	UNUSED(m_inputs, m_state);
}

/*****************************************************************************/
[[nodiscard]] Unique<CompileEnvironment> CompileEnvironment::make(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState)
{
	if (inType == ToolchainType::MSVC)
	{
		return std::make_unique<VisualStudioCompileEnvironment>(inInputs, inState);
	}
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
	else if (inType == ToolchainType::IntelClassic)
	{
		return std::make_unique<IntelCompileEnvironment>(inInputs, inState);
	}
#endif
#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
	else if (inType == ToolchainType::IntelLLVM)
	{
		return std::make_unique<IntelCompileEnvironment>(inInputs, inState);
	}
#endif

	return std::make_unique<CompileEnvironment>(inInputs, inState);
}

/*****************************************************************************/
const std::string& CompileEnvironment::detectedVersion() const
{
	return m_detectedVersion;
}

/*****************************************************************************/
bool CompileEnvironment::create(const std::string& inVersion)
{
	UNUSED(inVersion);

	if (m_initialized)
	{
		Diagnostic::error("Compiler environment was already initialized.");
		return false;
	}

	m_initialized = true;

	if (!createFromVersion(inVersion))
		return false;

	return true;
}

/*****************************************************************************/
bool CompileEnvironment::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);
	return true;
}

/*****************************************************************************/
std::string CompileEnvironment::getVarsPath(const std::string& inId) const
{
	auto archString = m_inputs.getArchWithOptionsAsString(m_state.info.targetArchitectureString());
	archString += fmt::format("_{}", m_inputs.toolchainPreferenceName());
	return m_state.cache.getHashPath(fmt::format("{}_{}.env", inId, archString), CacheType::Local);
}

/*****************************************************************************/
bool CompileEnvironment::saveOriginalEnvironment(const std::string& inOutputFile) const
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
void CompileEnvironment::createEnvironmentDelta(const std::string& inOriginalFile, const std::string& inCompilerFile, const std::string& inDeltaFile, const std::function<void(std::string&)>& onReadLine) const
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
void CompileEnvironment::cacheEnvironmentDelta(const std::string& inDeltaFile)
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
}
