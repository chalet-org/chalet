/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/MesonBuilder.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Dependency/GitDependency.hpp"
#include "State/Target/MesonTarget.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"
#include "Utility/Version.hpp"

namespace chalet
{
/*****************************************************************************/
MesonBuilder::MesonBuilder(const BuildState& inState, const MesonTarget& inTarget, const bool inQuotedPaths) :
	m_state(inState),
	m_target(inTarget),
	m_quotedPaths(inQuotedPaths)
{
	m_mesonVersionMajorMinor = m_state.toolchain.mesonVersionMajor();
	m_mesonVersionMajorMinor *= 100;
	m_mesonVersionMajorMinor += m_state.toolchain.mesonVersionMinor();
}

/*****************************************************************************/
std::string MesonBuilder::getLocation() const
{
	const auto& rawLocation = m_target.location();
	auto ret = Files::getAbsolutePath(rawLocation);
	Path::toUnix(ret);

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getBuildFile(const bool inForce) const
{
	std::string ret;
	if (!m_target.buildFile().empty())
	{
		auto location = getLocation();
		ret = fmt::format("{}/{}", location, m_target.buildFile());
	}
	else if (inForce)
	{
		auto location = getLocation();
		ret = fmt::format("{}/meson.build", location);
	}

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getCacheFile() const
{
	std::string ret;

	auto location = m_target.targetFolder();
	ret = fmt::format("{}/build.ninja", location);

	return ret;
}

/*****************************************************************************/
bool MesonBuilder::dependencyHasUpdated() const
{
	if (m_target.hashChanged())
	{
		Files::removeRecursively(m_target.targetFolder());
		return true;
	}

	return false;
}

/*****************************************************************************/
bool MesonBuilder::run()
{
	Timer buildTimer;

	const auto& name = m_target.name();

	const bool isNinja = usesNinja();

	static const char kNinjaStatus[] = "NINJA_STATUS";
	auto oldNinjaStatus = Environment::getString(kNinjaStatus);

	auto onRunFailure = [this, &oldNinjaStatus, &isNinja](const bool inRemoveDir = true) -> bool {
#if defined(CHALET_WIN32)
		Output::previousLine();
#endif

		if (inRemoveDir && !m_target.recheck())
			Files::removeRecursively(outputLocation());

		Output::lineBreak();

		if (isNinja)
			Environment::set(kNinjaStatus, oldNinjaStatus);

		return false;
	};

	auto& buildDir = outputLocation();

	auto& sourceCache = m_state.cache.file().sources();
	auto outputHash = Hash::string(buildDir);
	bool lastBuildFailed = sourceCache.dataCacheValueIsFalse(outputHash);
	bool dependencyUpdated = dependencyHasUpdated();

	bool outDirectoryDoesNotExist = !Files::pathExists(buildDir);
	bool recheckMeson = m_target.recheck() || lastBuildFailed || dependencyUpdated;

	if (outDirectoryDoesNotExist || recheckMeson)
	{
		if (outDirectoryDoesNotExist)
			Files::makeDirectory(buildDir);

		if (isNinja)
		{
			const auto& color = Output::getAnsiStyle(Output::theme().build);
			Environment::set(kNinjaStatus, fmt::format("   [%f/%t] {}", color));
		}

		StringList command;
		command = getSetupCommand();

		std::string cwd = m_mesonVersionMajorMinor >= 313 ? std::string() : buildDir;

		if (!Process::run(command, cwd))
			return onRunFailure();

		command = getBuildCommand(buildDir);

		bool result = Process::runNinjaBuild(command);
		sourceCache.addDataCache(outputHash, result);
		if (!result)
			return onRunFailure(false);

		if (isNinja)
		{
			Environment::set(kNinjaStatus, oldNinjaStatus);
		}
	}

	//
	Output::msgTargetUpToDate(name, &buildTimer);

	return true;
}

/*****************************************************************************/
std::string MesonBuilder::getBackend() const
{
	const bool isNinja = usesNinja();

	std::string ret;
	if (isNinja)
	{
		ret = "ninja";
	}
	else
	{
		ret = "none";
	}

	// vs,vs2017,vs2019,vs2022,xcode

	return ret;
}

/*****************************************************************************/
StringList MesonBuilder::getSetupCommand()
{
	auto location = getLocation();
	auto buildFile = getBuildFile();

	return getSetupCommand(location, buildFile);
}

/*****************************************************************************/
StringList MesonBuilder::getSetupCommand(const std::string& inLocation, const std::string& inBuildFile) const
{
	auto& meson = m_state.toolchain.meson();

	auto backend = getBackend();
	chalet_assert(!backend.empty(), "Meson Backend is empty");

	StringList ret{ getQuotedPath(meson), "setup", "--backend", backend };

	if (Output::showCommands())
		ret.emplace_back("--errorlogs");

	ret.emplace_back(getQuotedPath(inLocation));

	if (!inBuildFile.empty())
	{
		ret.emplace_back(getQuotedPath(inBuildFile));
	}
	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getMesonCompatibleBuildConfiguration() const
{
	std::string ret = m_state.configuration.name();

	if (m_state.configuration.isMinSizeRelease())
	{
		ret = "minsize";
	}
	else if (m_state.configuration.isReleaseWithDebugInfo())
	{
		ret = "debugoptimized";
	}
	else if (m_state.configuration.isDebuggable())
	{
		// Profile > debug in Meson
		ret = "debug";
	}
	else
	{
		// RelHighOpt > release in Meson
		ret = "release";
	}

	return ret;
}

/*****************************************************************************/
StringList MesonBuilder::getBuildCommand() const
{
	auto outputLocation = m_target.targetFolder();
	return getBuildCommand(outputLocation);
}

/*****************************************************************************/
StringList MesonBuilder::getBuildCommand(const std::string& inOutputLocation) const
{
	auto& meson = m_state.toolchain.meson();
	// const auto maxJobs = m_state.info.maxJobs();

	auto buildLocation = Files::getAbsolutePath(inOutputLocation);
	StringList ret{ getQuotedPath(meson), "compile", "-C", getQuotedPath(buildLocation) };

	// const auto& targets = m_target.targets();
	// if (!targets.empty())
	// {
	// 	ret.emplace_back("-t");
	// 	for (const auto& name : targets)
	// 	{
	// 		ret.emplace_back(name);
	// 	}
	// }

	// if (isNinja)
	// {
	// 	ret.emplace_back("--");

	// 	if (Output::showCommands())
	// 		ret.emplace_back("-v");

	// 	ret.emplace_back("-k");
	// 	ret.emplace_back(m_state.info.keepGoing() ? "0" : "1");
	// }

	// LOG(String::join(ret));

	return ret;
}

/*****************************************************************************/
std::string MesonBuilder::getQuotedPath(const std::string& inPath) const
{
	if (m_quotedPaths)
		return fmt::format("\"{}\"", inPath);
	else
		return inPath;
}

/*****************************************************************************/
bool MesonBuilder::usesNinja() const
{
	// Ignore the build strategy - Meson only supports ninja
	auto& ninjaExec = m_state.toolchain.ninja();
	if (ninjaExec.empty() || !Files::pathExists(ninjaExec))
		return false;

	return true;
}

/*****************************************************************************/
const std::string& MesonBuilder::outputLocation() const
{
	return m_target.targetFolder();
}

}
