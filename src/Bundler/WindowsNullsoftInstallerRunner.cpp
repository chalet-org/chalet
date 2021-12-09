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
#include "Terminal/Path.hpp"
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
	const auto& nsisScript = inTarget.nsisScript();

	Timer timer;
	if (Output::showCommands())
		Diagnostic::info("Creating the Windows installer executable");
	else
		Diagnostic::infoEllipsis("Creating the Windows installer executable");

	StringList cmd{ m_prototype.tools.makeNsis() };
	cmd.emplace_back("-WX");
	cmd.emplace_back("-V3");
	cmd.emplace_back("-NOCD");

	StringList resolvedPluginPaths = getPluginPaths(inTarget);
	for (auto& path : resolvedPluginPaths)
	{
		cmd.emplace_back(fmt::format("-X!addplugindir {}", path));
	}

	cmd.emplace_back(nsisScript);

	if (!Commands::subprocessMinimalOutput(cmd))
	{
		Diagnostic::error("NSIS Installer failed to compile: {}", nsisScript);
		return false;
	}

	Diagnostic::printDone(timer.asString());

	return true;
}

/*****************************************************************************/
StringList WindowsNullsoftInstallerRunner::getPluginPaths(const WindowsNullsoftInstallerTarget& inTarget)
{
	StringList ret;

	auto cwd = Commands::getWorkingDirectory();

	auto addCommonPluginFolders = [&ret](const std::string& inPath) -> bool {
		bool result = false;

		auto resolved = fmt::format("{}/amd64-unicode", inPath);
		if (Commands::pathExists(resolved))
		{
			ret.push_back(std::move(resolved));
			result = true;
		}

		resolved = fmt::format("{}/x86-ansi", inPath);
		if (Commands::pathExists(resolved))
		{
			ret.push_back(std::move(resolved));
			result = true;
		}

		resolved = fmt::format("{}/x86-unicode", inPath);
		if (Commands::pathExists(resolved))
		{
			ret.push_back(std::move(resolved));
			result = true;
		}

		return result;
	};

	for (auto& path : inTarget.pluginDirs())
	{
		auto pluginPath = fmt::format("{}/{}", cwd, path);
		Path::sanitizeForWindows(pluginPath);

		auto resolved = fmt::format("{}/Plugins", pluginPath);
		if (Commands::pathExists(resolved))
		{
			if (!addCommonPluginFolders(resolved))
				ret.push_back(std::move(resolved));
		}

		if (Commands::pathExists(pluginPath))
		{
			if (!addCommonPluginFolders(pluginPath))
				ret.push_back(std::move(pluginPath));
		}
	}

	return ret;
}
}
