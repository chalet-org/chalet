/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/IntelCompileEnvironment.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
IntelCompileEnvironment::IntelCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironment(inInputs, inState),
	kVarsId("intel")
{
}

/*****************************************************************************/
bool IntelCompileEnvironment::createFromVersion(const std::string& inVersion)
{
	UNUSED(inVersion);

	makeArchitectureCorrections();

	Timer timer;

	m_varsFileOriginal = m_state.cache.getHashPath(fmt::format("{}_original.env", kVarsId), CacheType::Local);
	m_varsFileIntel = m_state.cache.getHashPath(fmt::format("{}_all.env", kVarsId), CacheType::Local);
	m_varsFileIntelDelta = getVarsPath(kVarsId);
	m_path = Environment::getPath();

	bool isPresetFromInput = m_inputs.isToolchainPreset();

	bool deltaExists = Commands::pathExists(m_varsFileIntelDelta);
	if (!deltaExists)
	{
		Diagnostic::infoEllipsis("Creating Intel{} C/C++ Environment Cache", Unicode::registered());

		const auto& home = m_inputs.homeDirectory();
		m_intelSetVars = fmt::format("{}/intel/oneapi/setvars.sh", home);
		if (!Commands::pathExists(m_intelSetVars))
		{
			m_intelSetVars = "/opt/intel/oneapi/setvars.sh";
			if (!Commands::pathExists(m_intelSetVars))
			{
				Diagnostic::error("No suitable Intel C++ compiler installation found. Pleas install the Intel oneAPI Toolkit before continuing.");
				return false;
			}
		}

		// Read the current environment and save it to a file
		if (!saveOriginalEnvironment(m_varsFileOriginal))
		{
			Diagnostic::error("Intel Environment could not be fetched: The original environment could not be saved.");
			return false;
		}

		if (!saveIntelEnvironment())
		{
			Diagnostic::error("Intel Environment could not be fetched: The expected method returned with error.");
			return false;
		}

		createEnvironmentDelta(m_varsFileOriginal, m_varsFileIntel, m_varsFileIntelDelta, [this](std::string& line) {
			if (String::startsWith({ "PATH=", "Path=" }, line))
			{
				String::replaceAll(line, m_path, "");
			}
		});
	}
	else
	{
		Diagnostic::infoEllipsis("Reading Intel{} C/C++ Environment Cache", Unicode::registered());
	}

	// Read delta to cache
	cacheEnvironmentDelta(m_varsFileIntelDelta);

	StringList pathSearch{ "Path", "PATH" };
	for (auto& [name, var] : m_variables)
	{
		if (String::equals(pathSearch, name))
		{
			// if (String::contains(inject, m_path))
			// {
			// 	String::replaceAll(m_path, inject, var);
			// 	Environment::set(name.c_str(), m_path);
			// }
			// else
			{
				Environment::set(name.c_str(), m_path + ":" + var);
			}
		}
		else
		{
			Environment::set(name.c_str(), var);
		}
	}

	if (isPresetFromInput)
	{
#if defined(CHALET_MACOS)
		std::string name = fmt::format("{}-apple-darwin-intel64", m_inputs.targetArchitecture());
#else
		std::string name = fmt::format("{}-intel", m_inputs.targetArchitecture());
#endif
		m_inputs.setToolchainPreferenceName(std::move(name));
	}

	m_state.cache.file().addExtraHash(String::getPathFilename(m_varsFileIntelDelta));

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
void IntelCompileEnvironment::makeArchitectureCorrections()
{
	auto target = m_inputs.targetArchitecture();
	if (target.empty())
	{
		// Try to get the architecture from the name
		const auto& preferenceName = m_inputs.toolchainPreferenceName();
		auto regexResult = RegexPatterns::matchesTargetArchitectureWithResult(preferenceName);
		if (!regexResult.empty())
		{
			target = regexResult;
		}
		else
		{
			target = m_inputs.hostArchitecture();
		}
	}

	m_inputs.setTargetArchitecture(target);
	m_state.info.setTargetArchitecture(m_inputs.targetArchitecture());
}

/*****************************************************************************/
bool IntelCompileEnvironment::saveIntelEnvironment() const
{
	StringList bashCmd;
	bashCmd.emplace_back("source");
	bashCmd.push_back(m_intelSetVars);
	bashCmd.emplace_back("--force");
	bashCmd.emplace_back(">");
	bashCmd.emplace_back("/dev/null");
	bashCmd.emplace_back("&&");
	bashCmd.emplace_back("printenv");
	bashCmd.emplace_back(">");
	bashCmd.push_back(m_varsFileIntel);
	auto bashCmdString = String::join(bashCmd);

	auto sh = Commands::which("sh");

	StringList cmd;
	cmd.push_back(sh);
	cmd.emplace_back("-c");
	cmd.emplace_back(fmt::format("'{}'", bashCmdString));

	auto outCmd = String::join(cmd);
	bool result = std::system(outCmd.c_str()) == EXIT_SUCCESS;

	return result;
}
}
