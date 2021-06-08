/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BuildManager/SubChaletBuilder.hpp"

#include "Libraries/Format.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const std::string& inChaletExecutable, const bool inCleanOutput) :
	m_state(inState),
	m_target(inTarget),
	m_chaletExecutable(inChaletExecutable),
	m_cleanOutput(inCleanOutput)
{
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	const auto& name = m_target.name();
	const auto& location = m_target.location();
	const auto& buildConfiguration = m_state.info.buildConfiguration();

	if (!Commands::pathExists(m_chaletExecutable))
	{
		m_chaletExecutable = Commands::which("chalet");
		if (!Commands::pathExists(m_chaletExecutable))
		{
			Diagnostic::error("The path to the chalet executable could not be resolved (welp.)");
			return false;
		}
	}

	Output::msgBuild(buildConfiguration, name);
	Output::lineBreak();

	const auto oldPath = Environment::getPath();
	const auto oldWorkingDirectory = Commands::getWorkingDirectory();

	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("executable: {}", m_chaletExecutable), false);
	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("name: {}", name), false);
	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("location: {}", location), false);
	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	const auto cwd = Commands::getAbsolutePath(location);
	Output::displayStyledSymbol(Color::Blue, " ", fmt::format("new cwd: {}", cwd), false);

	// Commands::changeWorkingDirectory(cwd);

	bool result = true;
	if (!Commands::subprocess({ m_chaletExecutable }, cwd, nullptr, PipeOption::StdOut, PipeOption::StdErr, {}, m_cleanOutput))
		result = false;

	// Commands::changeWorkingDirectory(oldWorkingDirectory);
	Environment::setPath(oldPath);

	if (result)
	{
		Output::lineBreak();

		//
		Output::msgTargetUpToDate(m_state.targets.size() > 1, name);
	}

	return result;
}
}
