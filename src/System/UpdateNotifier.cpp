/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "System/UpdateNotifier.hpp"

#include "Cache/WorkspaceInternalCacheFile.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "State/CentralState.hpp"
#include "System/DefinesVersion.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Version.hpp"

namespace chalet
{
/*****************************************************************************/
UpdateNotifier::UpdateNotifier(const CentralState& inCentralState) :
	m_centralState(inCentralState)
{
}

/*****************************************************************************/
void UpdateNotifier::checkForUpdates(const CentralState& inCentralState)
{
	UpdateNotifier notifier(inCentralState);
	notifier.checkForUpdates();
}

/*****************************************************************************/
void UpdateNotifier::checkForUpdates()
{
	if (m_centralState.shouldPerformUpdateCheck())
	{
		std::string git = m_centralState.tools.git();
		if (git.empty())
			git = Files::which("git");

		if (git.empty())
			return;

		StringList cmd{
			git,
			"ls-remote",
			"--refs",
			"--tags",
			"https://github.com/chalet-org/chalet",
		};
		auto output = Process::runOutput(cmd);
		if (output.empty())
			return;

		std::vector<Version> versions;
		std::istringstream input(output);
		std::string line;
		auto lineEnd = input.widen('\n');
		while (std::getline(input, line, lineEnd))
		{
			if (line.empty())
				continue;

			auto search = line.find_last_of("refs/tags/v");
			if (search != std::string::npos)
			{
				auto ver = line.substr(search + 1);
				versions.emplace_back(Version::fromString(ver));
			}
		}

		if (!versions.empty())
		{
			std::sort(versions.begin(), versions.end(), [](Version& a, Version& b) {
				return a < b;
			});

			auto ver = Version::fromString(std::string(CHALET_VERSION));
			auto& lat = versions.back();
			if (ver < lat)
			{
				showUpdateMessage(ver.asString(), lat.asString());
			}
		}
	}
}

/*****************************************************************************/
void UpdateNotifier::showUpdateMessage(const std::string& inOld, const std::string& inNew) const
{
	auto& theme = Output::theme();
	const auto& dim = Output::getAnsiStyle(theme.flair);
	const auto& colOld = Output::getAnsiStyle(theme.build);
	const auto& colNew = Output::getAnsiStyle(theme.success);
	const auto& reset = Output::getAnsiStyle(theme.reset);

	const auto& route = m_centralState.inputs().route();
	if (route.isBuildRun() || route.isRun())
		Output::lineBreak();

	Diagnostic::info("Update available: {}{}{} -> {}{}{}\n   {}Get it from: https://www.chalet-work.space/download{}", colOld, inOld, reset, colNew, inNew, reset, dim, reset);
	Output::lineBreak();
}

}
