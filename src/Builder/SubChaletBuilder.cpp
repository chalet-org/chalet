/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/SubChaletBuilder.hpp"

#include "Core/CommandLineInputs.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/Path.hpp"
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
	auto ret = Files::getAbsolutePath(rawLocation);
	Path::toUnix(ret);

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
	const auto& name = m_target.name();

	const auto oldPath = Environment::getPath();

	Environment::set("__CHALET_PARENT_CWD", m_state.inputs.workingDirectory() + '/');
	Environment::set("__CHALET_TARGET", "1");

	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("executable: {}", m_state.inputs.appPath()), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("name: {}", name), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("location: {}", location), false);
	// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("cwd: {}", oldWorkingDirectory), false);

	auto& sourceCache = m_state.cache.file().sources();
	auto outputHash = Hash::string(outputLocation());
	bool lastBuildFailed = sourceCache.dataCacheValueIsFalse(outputHash);
	bool dependencyUpdated = dependencyHasUpdate();

	bool outDirectoryDoesNotExist = !Files::pathExists(outputLocation());
	bool recheckChalet = m_target.recheck() || lastBuildFailed || dependencyUpdated;

	auto onRunFailure = [&oldPath]() -> bool {
		Environment::setPath(oldPath);
		Environment::set("__CHALET_PARENT_CWD", std::string());
		Environment::set("__CHALET_TARGET", std::string());

		Output::lineBreak();
		return false;
	};

	bool result = true;

	if (outDirectoryDoesNotExist || recheckChalet)
	{
		// Output::displayStyledSymbol(Output::theme().info, " ", fmt::format("new cwd: {}", cwd), false);

		auto cmd = getBuildCommand();
		result = Process::run(cmd);
		if (!result)
			return onRunFailure();

		sourceCache.addDataCache(outputHash, result);
	}

	if (result)
	{
		Environment::setPath(oldPath);

		bool clean = m_state.inputs.route().isClean() && m_target.clean();
		if (!clean)
		{
			Output::msgTargetUpToDate(m_state.targets.size() > 1, name);
		}
	}
	else
	{
		return onRunFailure();
	}

	return result;
}

/*****************************************************************************/
void SubChaletBuilder::removeSettingsFile()
{
	auto location = getLocation();
	auto& defaultSettingsFile = m_state.inputs.defaultSettingsFile();
	auto settingsLocation = fmt::format("{}/{}", location, defaultSettingsFile);

	if (Files::pathExists(settingsLocation))
		Files::remove(settingsLocation);
}

/*****************************************************************************/
StringList SubChaletBuilder::getBuildCommand(const bool hasSettings) const
{
	auto location = getLocation();
	auto buildFile = getBuildFile();

	return getBuildCommand(location, buildFile, hasSettings);
}

/*****************************************************************************/
StringList SubChaletBuilder::getBuildCommand(const std::string& inLocation, const std::string& inBuildFile, const bool hasSettings) const
{
	StringList cmd{ getQuotedPath(m_state.inputs.appPath()) };
	cmd.emplace_back("--quieter");

	auto outputDirectory = Files::getCanonicalPath(outputLocation());

	cmd.emplace_back("--root-dir");
	cmd.push_back(getQuotedPath(inLocation));

	if (!inBuildFile.empty())
	{
		cmd.emplace_back("--input-file");
		cmd.push_back(getQuotedPath(inBuildFile));
	}

	if (!hasSettings)
	{
		auto proximateSettings = Files::getCanonicalPath(m_state.inputs.settingsFile());

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
		if (Files::pathExists(m_state.inputs.envFile()))
		{
			auto envAbsolute = Files::getAbsolutePath(m_state.inputs.envFile());
			cmd.emplace_back("--env-file");
			cmd.push_back(getQuotedPath(envAbsolute));
		}
	}

	if (!m_state.inputs.architectureRaw().empty())
	{
		cmd.emplace_back("--arch");
		cmd.push_back(m_state.inputs.architectureRaw());
	}

	// We'll use the native strategy because ninja doesn't like absolute paths on Windows
	cmd.emplace_back("--build-strategy");
	cmd.push_back(m_state.toolchain.getStrategyString());

	if (Output::showCommands())
		cmd.emplace_back("--show-commands");
	else
		cmd.emplace_back("--no-show-commands");

	cmd.emplace_back("--only-required");

	bool rebuild = m_state.inputs.route().isRebuild() && m_target.rebuild();
	bool clean = m_state.inputs.route().isClean() && m_target.clean();

	if (rebuild)
		cmd.emplace_back("rebuild");
	else if (clean)
		cmd.emplace_back("clean");
	else
		cmd.emplace_back("build");

	if (!clean)
	{
		auto target = String::join(m_target.targets(), ',');
		cmd.emplace_back(target);
	}

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

/*****************************************************************************/
const std::string& SubChaletBuilder::outputLocation() const
{
	return m_target.targetFolder();
}
}
