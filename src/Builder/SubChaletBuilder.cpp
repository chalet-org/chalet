/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/SubChaletBuilder.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Libraries/Format.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const CommandLineInputs& inInputs) :
	m_state(inState),
	m_target(inTarget),
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	const auto& name = m_target.name();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	const auto oldPath = Environment::getPath();
	const auto oldWorkingDirectory = Commands::getWorkingDirectory();

	auto cwd = Commands::getWorkingDirectory();
	auto location = fmt::format("{}/{}", cwd, m_target.location());
	Path::sanitize(location);
	if (!m_target.buildFile().empty())
	{
		m_buildFile = fmt::format("{}/{}", location, m_target.buildFile());
	}

	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("executable: {}", m_state.ancillaryTools.chalet()), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("name: {}", name), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("location: {}", location), false);
	// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	m_outputLocation = fmt::format("{}/{}", location, buildOutputDir);
	Path::sanitize(m_outputLocation);

	bool outDirectoryDoesNotExist = !Commands::pathExists(m_outputLocation);
	bool recheckChalet = m_target.recheck();

	bool result = true;

	if (outDirectoryDoesNotExist || recheckChalet)
	{
		// Output::displayStyledSymbol(Color::Blue, " ", fmt::format("new cwd: {}", cwd), false);

		// Commands::changeWorkingDirectory(workingDirectory);

		StringList cmd = getBuildCommand();
		result = Commands::subprocess(cmd, location);

		// Commands::changeWorkingDirectory(oldWorkingDirectory);
		Environment::setPath(oldPath);
	}

	if (result)
	{
		//
		Output::msgTargetUpToDate(m_state.targets.size() > 1, name);
	}

	return result;
}

/*****************************************************************************/
StringList SubChaletBuilder::getBuildCommand() const
{
	StringList cmd{ m_state.ancillaryTools.chalet() };
	cmd.push_back("--quieter");

	if (!m_buildFile.empty())
	{
		cmd.push_back("--input-file");
		cmd.push_back(m_buildFile);
	}

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

	return cmd;
}
}
