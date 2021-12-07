/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/WindowsNullsoftInstallerRunner.hpp"

#include "State/AncillaryTools.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
WindowsNullsoftInstallerRunner::WindowsNullsoftInstallerRunner(const StatePrototype& inPrototype) :
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool WindowsNullsoftInstallerRunner::compile(const WindowsNullsoftInstallerTarget& inTarget)
{
	// TODO: ignore on CI for now
	//   plugins will need to be defined as well as the script, and then the plugins
	//   will need to be copied to the NSIS install directory (it's that old school)
	//
	if (Environment::isContinuousIntegrationServer())
		return true;

	const auto& nsisScript = inTarget.nsisScript();
	if (!nsisScript.empty())
	{
		Timer timer;
		if (Output::showCommands())
			Diagnostic::info("Creating the Windows installer executable");
		else
			Diagnostic::infoEllipsis("Creating the Windows installer executable");

		StringList cmd{ m_prototype.tools.makeNsis() };
		cmd.emplace_back("/WX");
		cmd.emplace_back("/V3");
		cmd.emplace_back("/NOCD");
		cmd.emplace_back(nsisScript);

		bool result = false;
		if (Output::showCommands())
			result = Commands::subprocess(cmd);
		else
			result = Commands::subprocess(cmd, PipeOption::Close, PipeOption::StdOut);

		if (!result)
		{
			Diagnostic::error("NSIS Installer failed to compile: {}", nsisScript);
			return false;
		}

		Diagnostic::printDone(timer.asString());
	}

	return true;
}
}
