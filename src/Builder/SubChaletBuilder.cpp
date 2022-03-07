/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/SubChaletBuilder.hpp"

#include "Core/CommandLineInputs.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
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
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	m_buildFile.clear();
	m_outputLocation.clear();

	const auto& name = m_target.name();

	Output::msgBuild(name);
	Output::lineBreak();

	const auto oldPath = Environment::getPath();

	auto location = Commands::getAbsolutePath(m_target.location());
	Path::sanitize(location);

	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	m_outputLocation = fmt::format("{}/{}", location, buildOutputDir);
	Path::sanitize(m_outputLocation);

	if (!m_target.buildFile().empty())
	{
		m_buildFile = fmt::format("{}/{}", location, m_target.buildFile());
	}
	else
	{
		m_buildFile = fmt::format("{}/{}", location, m_state.inputs.defaultInputFile());
	}

	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("executable: {}", m_state.tools.chalet()), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("name: {}", name), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("location: {}", location), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	auto& sourceCache = m_state.cache.file().sources();
	bool lastBuildFailed = sourceCache.externalRequiresRebuild(m_target.location());

	bool outDirectoryDoesNotExist = !Commands::pathExists(m_outputLocation);
	bool recheckChalet = m_target.recheck() || lastBuildFailed;

	bool result = true;

	if (outDirectoryDoesNotExist || recheckChalet)
	{
		// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("new cwd: {}", cwd), false);

		// Commands::changeWorkingDirectory(workingDirectory);

		StringList cmd = getBuildCommand(location);
		result = Commands::subprocess(cmd);
		sourceCache.addExternalRebuild(m_target.location(), result ? "0" : "1");

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
StringList SubChaletBuilder::getBuildCommand(const std::string& inLocation) const
{
	StringList cmd{ m_state.tools.chalet() };
	cmd.emplace_back("build");
	cmd.emplace_back("--quieter");

	auto proximateOutput = Commands::getProximatePath(m_state.inputs.outputDirectory(), inLocation);
	auto proximateSettings = Commands::getProximatePath(m_state.inputs.settingsFile(), inLocation);

	cmd.emplace_back("--root-dir");
	cmd.push_back(inLocation);

	if (!m_buildFile.empty())
	{
		cmd.emplace_back("--input-file");
		cmd.push_back(m_buildFile);
	}

	cmd.emplace_back("--settings-file");
	cmd.emplace_back(std::move(proximateSettings));

	cmd.emplace_back("--external-dir");
	cmd.push_back(m_state.inputs.externalDirectory());

	cmd.emplace_back("--output-dir");
	cmd.emplace_back(fmt::format("{}/{}", proximateOutput, m_target.name()));

	cmd.emplace_back("--configuration");
	cmd.push_back(m_state.info.buildConfiguration());

	if (!m_state.inputs.toolchainPreferenceName().empty())
	{
		cmd.emplace_back("--toolchain");
		cmd.push_back(m_state.inputs.toolchainPreferenceName());
	}

	if (!m_state.inputs.envFile().empty())
	{
		if (Commands::pathExists(m_state.inputs.envFile()))
		{
			auto envAbsolute = Commands::getAbsolutePath(m_state.inputs.envFile());
			cmd.emplace_back("--env-file");
			cmd.push_back(envAbsolute);
		}
	}

	if (!m_state.inputs.architectureRaw().empty())
	{
		cmd.emplace_back("--arch");
		cmd.push_back(m_state.inputs.architectureRaw());
	}

	return cmd;
}
}
