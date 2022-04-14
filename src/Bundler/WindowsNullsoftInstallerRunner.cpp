/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Bundler/WindowsNullsoftInstallerRunner.hpp"

#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Distribution/WindowsNullsoftInstallerTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
WindowsNullsoftInstallerRunner::WindowsNullsoftInstallerRunner(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
bool WindowsNullsoftInstallerRunner::compile(const WindowsNullsoftInstallerTarget& inTarget)
{
	const auto& file = inTarget.file();
	chalet_assert(!file.empty(), "WindowsNullsoftInstallerRunner: validate target first");

	StringList cmd{ m_state.tools.makeNsis() };
	cmd.emplace_back("-WX");
	cmd.emplace_back("-V3");
	cmd.emplace_back("-NOCD");

	StringList definePrefixes{ "-D", "/D" };

	for (auto& define : inTarget.defines())
	{
		if (String::startsWith(definePrefixes, define))
			cmd.emplace_back(define);
		else
			cmd.emplace_back(fmt::format("-D{}", define));
	}

	StringList resolvedPluginPaths = getPluginPaths(inTarget);
	for (auto& path : resolvedPluginPaths)
	{
#if defined(CHALET_WIN32)
		Path::sanitizeForWindows(path);
#endif
		cmd.emplace_back(fmt::format("-X!addplugindir \"{}\"", path));
	}

	cmd.emplace_back(file);

	if (!Commands::subprocessMinimalOutput(cmd))
	{
		Diagnostic::error("NSIS Installer failed to compile: {}", file);
		return false;
	}

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
		bool checkRoot = true;
		auto resolved = fmt::format("{}/{}", cwd, path);
		Path::sanitizeForWindows(resolved);

		auto pluginsPath = fmt::format("{}/Plugins", resolved);
		if (Commands::pathExists(pluginsPath))
		{
			checkRoot = false;
			if (!addCommonPluginFolders(pluginsPath))
				ret.push_back(std::move(pluginsPath));
			else
				checkRoot = true;
		}

		if (checkRoot && Commands::pathExists(resolved))
		{
			if (!addCommonPluginFolders(resolved))
				ret.push_back(std::move(resolved));
		}
	}

	return ret;
}
}
