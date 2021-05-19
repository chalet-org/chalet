/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

// #define CHALET_KEEP_OLD_CACHE 1

namespace chalet
{
/*****************************************************************************/
CompileStrategyMakefile::CompileStrategyMakefile(BuildState& inState) :
	ICompileStrategy(StrategyType::Makefile, inState)
{
}

/*****************************************************************************/
bool CompileStrategyMakefile::initialize()
{
	if (m_initialized)
		return false;

	auto& name = "makefile";
	m_cacheFile = m_state.cache.getHash(name, BuildCache::Type::Local);

	auto& environmentCache = m_state.cache.environmentCache();
	Json& buildCache = environmentCache.json["data"];
	std::string key = fmt::format("{}:{}", m_state.buildConfiguration(), name);

	std::string existingHash;
	if (buildCache.contains(key))
	{
		existingHash = buildCache.at(key);
	}

	const bool cacheExists = Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(m_cacheFile);

	m_cacheNeedsUpdate = existingHash != hash || !cacheExists || appBuildChanged;

	if (m_cacheNeedsUpdate)
	{
		buildCache[key] = String::getPathFilename(m_cacheFile);
		m_state.cache.setDirty(true);
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::addProject(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain)
{
	if (!m_initialized)
		return false;

	if (m_hashes.find(inProject.name()) == m_hashes.end())
	{
		m_hashes.emplace(inProject.name(), Hash::string(inOutputs.target));
	}

	if (m_cacheNeedsUpdate)
	{
		auto& hash = m_hashes.at(inProject.name());
		m_generator->addProjectRecipes(inProject, inOutputs, inToolchain, hash);
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::saveBuildFile() const
{
	if (!m_initialized || !m_generator->hasProjectRecipes())
		return false;

	if (m_cacheNeedsUpdate)
	{
		std::ofstream(m_cacheFile) << m_generator->getContents(m_cacheFile)
								   << std::endl;
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::buildProject(const ProjectConfiguration& inProject) const
{
	const auto& makeExec = m_state.tools.make();
	if (makeExec.empty() || !Commands::pathExists(makeExec))
	{
		Diagnostic::error(fmt::format("{} was not found in compiler path. Aborting.", makeExec));
		return false;
	}

	if (m_hashes.find(inProject.name()) == m_hashes.end())
	{
		Diagnostic::error(fmt::format("{} was not previously cached. Aborting.", inProject.name()));
		return false;
	}

	StringList command;
	const auto maxJobs = m_state.environment.maxJobs();

#if defined(CHALET_WIN32)
	const bool isNMake = m_state.tools.isNMake();
	if (isNMake)
	{
		command.clear();
		command.push_back(makeExec);
		command.push_back("/NOLOGO");
		command.push_back("/F");
		command.push_back(m_cacheFile);
	}
	else
#endif
	{
		std::string jobs;
		if (maxJobs > 0)
			jobs = fmt::format("-j{}", maxJobs);

		command.clear();
		command.push_back(makeExec);
		if (!jobs.empty())
		{
			command.push_back(jobs);
		}
		// m_makeCmd.push_back("-d");
		command.push_back("-C");
		command.push_back(".");
		command.push_back("-f");
		command.push_back(m_cacheFile);
		command.push_back("--no-print-directory");
	}

	auto& hash = m_hashes.at(inProject.name());

	{
		// std::cout << Output::getAnsiStyle(Color::Blue) << std::flush;

		command.push_back(fmt::format("build_{}", hash));
		bool result = Commands::subprocess(command, PipeOption::StdOut);

		if (!result)
			return false;
	}

	if (inProject.dumpAssembly())
	{
		// std::cout << Output::getAnsiStyle(Color::Magenta) << std::flush;

		command.back() = fmt::format("asm_{}", hash);

		bool result = Commands::subprocess(command, PipeOption::StdOut);

		if (!result)
			return false;
	}

	return true;
}
}
