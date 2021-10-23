/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentGNU::CompileEnvironmentGNU(const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentGNU::type() const noexcept
{
	return ToolchainType::GNU;
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
		std::string arch;
		for (auto& line : splitOutput)
		{
			parseVersionFromVersionOutput(line, version);
			parseArchFromVersionOutput(line, arch);
			// parseThreadModelFromVersionOutput(line, threadModel);
		}

		version = version.substr(0, version.find_first_not_of("0123456789."));
		if (!version.empty())
		{
			ret.description = getFullCxxCompilerString(version);
			ret.version = std::move(version);
		}
		else
		{
			ret.description = "Unrecognized";
		}

		if (!arch.empty())
			ret.arch = std::move(arch);
		else
			ret.arch = m_state.info.targetArchitectureTriple();
	}

	return ret;
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
		auto result = Commands::subprocessOutput({ compiler, "-dumpmachine" });
#if defined(CHALET_MACOS)
		// Strip out version in auto-detected mac triple
		auto darwin = result.find("apple-darwin");
		if (darwin != std::string::npos)
		{
			result = result.substr(0, darwin + 12);
		}
#endif
		m_state.info.setTargetArchitecture(result);
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
}