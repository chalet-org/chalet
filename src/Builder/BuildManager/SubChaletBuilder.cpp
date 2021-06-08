/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/SubChaletBuilder.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const CommandLineInputs& inInputs, const bool inCleanOutput) :
	m_state(inState),
	m_target(inTarget),
	m_inputs(inInputs),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	const auto& name = m_target.name();
	const auto& location = m_target.location();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	std::string exec = m_inputs.appPath();

	if (!Commands::pathExists(exec))
	{
		exec = Commands::which("chalet");
		if (!Commands::pathExists(exec))
		{
			Diagnostic::error("The path to the chalet executable could not be resolved (welp.)");
			return false;
		}
	}

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	const auto oldPath = Environment::getPath();
	const auto oldWorkingDirectory = Commands::getWorkingDirectory();

	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("executable: {}", exec), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("name: {}", name), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("location: {}", location), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	const auto cwd = Commands::getAbsolutePath(location);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("new cwd: {}", cwd), false);

	// Commands::changeWorkingDirectory(cwd);

	bool result = true;

	StringList cmd{ exec };
	cmd.push_back("--quieter");

	if (!m_inputs.toolchainPreferenceRaw().empty())
	{
		cmd.push_back("--toolchain");
		cmd.push_back(m_inputs.toolchainPreferenceRaw());
	}

	if (!m_inputs.envFile().empty())
	{
		if (Commands::pathExists(m_inputs.envFile()))
		{
			auto envAbsolute = Commands::getAbsolutePath(m_inputs.envFile());
			cmd.push_back("--envfile");
			cmd.push_back(envAbsolute);
		}
	}

	if (!m_inputs.archRaw().empty())
	{
		cmd.push_back("--arch");
		cmd.push_back(m_inputs.archRaw());
	}

	cmd.push_back("build");
	cmd.push_back(m_state.info.buildConfiguration());

	// UNUSED(m_cleanOutput);
	if (!Commands::subprocess(cmd, cwd, m_cleanOutput))
		result = false;

	// Commands::changeWorkingDirectory(oldWorkingDirectory);
	Environment::setPath(oldPath);

	if (result)
	{
		//
		Output::msgTargetUpToDate(m_state.targets.size() > 1, name);
	}

	return result;
}
}
