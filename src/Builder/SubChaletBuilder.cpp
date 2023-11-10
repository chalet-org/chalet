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
#include "State/CompilerTools.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Process/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SubChaletBuilder::SubChaletBuilder(const BuildState& inState, const SubChaletTarget& inTarget, const bool inQuotedPaths) :
	m_state(inState),
	m_target(inTarget),
	m_quotedPaths(inQuotedPaths)
{
}

/*****************************************************************************/
std::string SubChaletBuilder::getLocation() const
{
	const auto& rawLocation = m_target.location();
	auto ret = Commands::getAbsolutePath(rawLocation);
	Path::sanitize(ret);

	return ret;
}

/*****************************************************************************/
std::string SubChaletBuilder::getOutputLocation() const
{
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto location = getLocation();

	auto ret = fmt::format("{}/{}", location, buildOutputDir);
	Path::sanitize(ret);

	return ret;
}

/*****************************************************************************/
std::string SubChaletBuilder::getBuildFile() const
{
	std::string ret;

	auto location = getLocation();
	if (!m_target.buildFile().empty())
	{
		ret = fmt::format("{}/{}", location, m_target.buildFile());
	}
	else
	{
		ret = fmt::format("{}/{}", location, m_state.inputs.defaultInputFile());
	}

	return ret;
}

/*****************************************************************************/
bool SubChaletBuilder::dependencyHasUpdate() const
{
	bool updated = false;
	for (const auto& dependency : m_state.externalDependencies)
	{
		if (dependency->isGit())
		{
			auto& gitDependency = static_cast<GitDependency&>(*dependency);
			if (String::startsWith(gitDependency.destination(), m_target.location()))
			{
				updated = gitDependency.needsUpdate();
			}
		}
	}

	return updated;
}

/*****************************************************************************/
bool SubChaletBuilder::run()
{
	m_outputLocation.clear();

	const auto& name = m_target.name();

	m_outputLocation = getOutputLocation();

	const auto oldPath = Environment::getPath();

	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("executable: {}", m_state.tools.chalet()), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("name: {}", name), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("location: {}", location), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	auto& sourceCache = m_state.cache.file().sources();
	bool lastBuildFailed = sourceCache.externalRequiresRebuild(m_target.targetFolder());
	bool strategyChanged = m_state.cache.file().buildStrategyChanged();
	bool dependencyUpdated = dependencyHasUpdate();

	if (strategyChanged)
		Commands::removeRecursively(m_outputLocation);

	bool outDirectoryDoesNotExist = !Commands::pathExists(m_outputLocation);
	bool recheckChalet = m_target.recheck() || lastBuildFailed || strategyChanged || dependencyUpdated;

	auto onRunFailure = [&oldPath]() -> bool {
		Environment::setPath(oldPath);
		Output::lineBreak();
		return false;
	};

	bool result = true;

	if (outDirectoryDoesNotExist || recheckChalet)
	{
		// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("new cwd: {}", cwd), false);

		for (auto& targetName : m_target.targets())
		{
			auto cmd = getBuildCommand(targetName);
			result = Commands::subprocess(cmd);
			if (!result)
				return onRunFailure();
		}
		sourceCache.addExternalRebuild(m_target.targetFolder(), result ? "0" : "1");
	}

	if (result)
	{
		Environment::setPath(oldPath);
		Output::msgTargetUpToDate(m_state.targets.size() > 1, name);
	}
	else
	{
		return onRunFailure();
	}

	return result;
}

/*****************************************************************************/
StringList SubChaletBuilder::getBuildCommand(const std::string& inTarget, const bool hasSettings) const
{
	auto location = getLocation();
	auto buildFile = getBuildFile();

	return getBuildCommand(location, buildFile, inTarget, hasSettings);
}

/*****************************************************************************/
StringList SubChaletBuilder::getBuildCommand(const std::string& inLocation, const std::string& inBuildFile, const std::string& inTarget, const bool hasSettings) const
{
	StringList cmd{ getQuotedPath(m_state.tools.chalet()) };
	cmd.emplace_back("--quieter");

	auto proximateOutput = Commands::getProximatePath(m_state.inputs.outputDirectory(), inLocation);
	auto outputDirectory = fmt::format("{}/{}", proximateOutput, m_target.name());

	cmd.emplace_back("--root-dir");
	cmd.push_back(getQuotedPath(inLocation));

	if (!inBuildFile.empty())
	{
		cmd.emplace_back("--input-file");
		cmd.push_back(getQuotedPath(inBuildFile));
	}

	if (!hasSettings)
	{
		auto proximateSettings = Commands::getProximatePath(m_state.inputs.settingsFile(), inLocation);

		cmd.emplace_back("--settings-file");
		cmd.emplace_back(getQuotedPath(proximateSettings));
	}

	cmd.emplace_back("--external-dir");
	cmd.push_back(getQuotedPath(m_state.inputs.externalDirectory()));

	cmd.emplace_back("--output-dir");
	cmd.emplace_back(getQuotedPath(outputDirectory));

	cmd.emplace_back("--configuration");
	cmd.push_back(m_state.info.buildConfiguration());

	if (!m_state.inputs.toolchainPreferenceName().empty())
	{
		cmd.emplace_back("--toolchain");
		cmd.push_back(getQuotedPath(m_state.inputs.toolchainPreferenceName()));
	}

	if (!m_state.inputs.envFile().empty())
	{
		if (Commands::pathExists(m_state.inputs.envFile()))
		{
			auto envAbsolute = Commands::getAbsolutePath(m_state.inputs.envFile());
			cmd.emplace_back("--env-file");
			cmd.push_back(getQuotedPath(envAbsolute));
		}
	}

	if (!m_state.inputs.architectureRaw().empty())
	{
		cmd.emplace_back("--arch");
		cmd.push_back(m_state.inputs.architectureRaw());
	}

	cmd.emplace_back("--only-required");
	cmd.emplace_back("build");
	cmd.emplace_back(inTarget);

	return cmd;
}

/*****************************************************************************/
std::string SubChaletBuilder::getQuotedPath(const std::string& inPath) const
{
	if (m_quotedPaths)
		return fmt::format("\"{}\"", inPath);
	else
		return inPath;
}

}
