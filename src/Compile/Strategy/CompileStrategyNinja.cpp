/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Process/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyNinja::CompileStrategyNinja(BuildState& inState) :
	ICompileStrategy(StrategyType::Ninja, inState)
{
}

/*****************************************************************************/
bool CompileStrategyNinja::initialize()
{
	if (m_initialized)
		return false;

	const auto& cachePathId = m_state.cachePathId();

	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(cachePathId, CacheType::Local);
	m_cacheFile = fmt::format("{}/build.ninja", m_cacheFolder);

	const bool cacheExists = Files::pathExists(m_cacheFolder) && Files::pathExists(m_cacheFile);
	const bool appVersionChanged = cacheFile.appVersionChanged();
	const bool themeChanged = cacheFile.themeChanged();
	const bool buildFileChanged = cacheFile.buildFileChanged();
	const bool buildHashChanged = cacheFile.buildHashChanged();
	const bool buildStrategyChanged = cacheFile.buildStrategyChanged();
	if (buildStrategyChanged)
	{
		Files::removeRecursively(m_state.paths.buildOutputDir());
	}

	m_cacheNeedsUpdate = !cacheExists || appVersionChanged || buildHashChanged || buildFileChanged || buildStrategyChanged || themeChanged;

	if (!Files::pathExists(m_cacheFolder))
		Files::makeDirectory(m_cacheFolder);

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::addProject(const SourceTarget& inProject)
{
	if (!m_initialized)
		return false;

	auto& name = inProject.name();
	const auto& outputs = m_outputs.at(name);
	if (m_hashes.find(name) == m_hashes.end())
	{
		m_hashes.emplace(name, Hash::string(outputs->target));
	}

	if (m_cacheNeedsUpdate)
	{
		auto& hash = m_hashes.at(name);
		auto& toolchain = m_toolchains.at(name);
		m_generator->addProjectRecipes(inProject, *outputs, toolchain, hash);
	}

	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyNinja::doPreBuild()
{
	if (m_initialized && m_generator->hasProjectRecipes())
	{
		if (m_cacheNeedsUpdate)
		{
			std::ofstream(m_cacheFile) << m_generator->getContents(m_cacheFolder)
									   << std::endl;
		}
	}

	return ICompileStrategy::doPreBuild();
}

/*****************************************************************************/
bool CompileStrategyNinja::buildProject(const SourceTarget& inProject)
{
	auto& ninjaExec = m_state.toolchain.ninja();
	if (m_hashes.find(inProject.name()) == m_hashes.end())
		return false;

	StringList command;
	command.push_back(ninjaExec);

	if (Output::showCommands())
		command.emplace_back("-v");

	command.emplace_back("-j");
	command.emplace_back(std::to_string(m_state.info.maxJobs()));

	command.emplace_back("-k");
	command.push_back(m_state.info.keepGoing() ? "0" : "1");

	command.emplace_back("-f");
	command.push_back(m_cacheFile);

	if (m_state.toolchain.ninjaVersionMajor() >= 1 && m_state.toolchain.ninjaVersionMinor() >= 10 && m_state.toolchain.ninjaVersionPatch() > 2)
	{
		// silences ninja status updates
		command.emplace_back("--quiet"); // forthcoming (in ninja's master branch currently)
	}

	command.emplace_back("-d");
	command.emplace_back("keepdepfile");

	auto& hash = m_hashes.at(inProject.name());

	static const char* kNinjaStatus = "NINJA_STATUS";
	auto oldNinjaStatus = Environment::getString(kNinjaStatus);

	auto color = Output::getAnsiStyle(Output::theme().build);
	Environment::set(kNinjaStatus, fmt::format("   [%f/%t] {}", color));

	command.emplace_back(fmt::format("build_{}", hash));
	bool result = Files::subprocessNinjaBuild(command);

	Environment::set(kNinjaStatus, oldNinjaStatus);

	// if (!result)
	// {
	// 	Output::lineBreak();
	// }

	return result;
}
}
