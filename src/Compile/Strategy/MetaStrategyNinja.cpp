/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/MetaStrategyNinja.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MetaStrategyNinja::MetaStrategyNinja(BuildState& inState) :
	m_state(inState),
	m_generator(inState)
{
}

/*****************************************************************************/
bool MetaStrategyNinja::initialize()
{
	if (m_initialized)
		return false;

	auto& name = "ninja";
	m_cacheFolder = m_state.cache.getHash(name, BuildCache::Type::Local);

	auto& environmentCache = m_state.cache.environmentCache();
	Json& buildCache = environmentCache.json["data"];
	std::string key = fmt::format("{}:{}", m_state.buildConfiguration(), name);

	std::string existingHash;
	if (buildCache.contains(key))
	{
		existingHash = buildCache.at(key);
	}

	m_cacheFile = fmt::format("{}/build.ninja", m_cacheFolder);
	const bool cacheExists = Commands::pathExists(m_cacheFolder) && Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(m_cacheFolder);

	m_cacheNeedsUpdate = existingHash != hash || !cacheExists || appBuildChanged;

	if (m_cacheNeedsUpdate)
	{
		if (!cacheExists)
		{
			Commands::makeDirectory(m_cacheFolder);
		}

		buildCache[key] = String::getPathFilename(m_cacheFolder);
		m_state.cache.setDirty(true);
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool MetaStrategyNinja::addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain)
{
	if (!m_initialized)
		return false;

	if (m_cacheNeedsUpdate)
	{
		if (m_hashes.find(inProject.name()) == m_hashes.end())
		{
			m_hashes.emplace(inProject.name(), Hash::string(inOutputs.target));
		}

		auto& hash = m_hashes.at(inProject.name());
		m_generator.addProjectRecipes(inProject, inOutputs, inToolchain, hash);
	}

	return true;
}

/*****************************************************************************/
bool MetaStrategyNinja::saveBuildFile() const
{
	if (!m_initialized || !m_generator.hasProjectRecipes())
		return false;

	if (m_cacheNeedsUpdate)
	{
		std::ofstream(m_cacheFile) << m_generator.getContents(m_cacheFolder)
								   << std::endl;
	}

	return true;
}

/*****************************************************************************/
bool MetaStrategyNinja::buildProject(const ProjectConfiguration& inProject)
{
	auto& ninjaExec = m_state.tools.ninja();
	if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
	{
		Diagnostic::error(fmt::format("{} was not found in compiler path. Aborting.", ninjaExec));
		return false;
	}

	if (m_hashes.find(inProject.name()) == m_hashes.end())
	{
		Diagnostic::error(fmt::format("{} was not found added to the ninja build file. Aborting.", inProject.name()));
		return false;
	}

	StringList command;
	command.push_back(ninjaExec);
	command.push_back("-f");
	command.push_back(m_cacheFile);

	auto& hash = m_hashes.at(inProject.name());

	{
		std::cout << Output::getAnsiStyle(Color::Blue) << std::flush;

		command.push_back(fmt::format("build_{}", hash));
		bool result = Commands::subprocess(command, PipeOption::StdOut);
		Output::lineBreak();

		if (!result)
			return false;
	}

	if (inProject.dumpAssembly())
	{
		std::cout << Output::getAnsiStyle(Color::Magenta) << std::flush;

		command.back() = fmt::format("asm_{}", hash);

		bool result = Commands::subprocess(command, PipeOption::StdOut);
		Output::lineBreak();

		if (!result)
			return false;
	}

	return true;
}
}
