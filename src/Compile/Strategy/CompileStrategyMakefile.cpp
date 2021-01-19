/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Compile/Strategy/MakefileGenerator.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/***********************4******************************************************/
CompileStrategyMakefile::CompileStrategyMakefile(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
}

/*****************************************************************************/
StrategyType CompileStrategyMakefile::type() const
{
	return StrategyType::Makefile;
}

/*****************************************************************************/
bool CompileStrategyMakefile::createCache(const SourceOutputs& inOutputs)
{
	const bool dumpAssembly = m_project.dumpAssembly();
	m_state.paths.setBuildEnvironment(inOutputs, dumpAssembly);

	auto& name = m_project.name();
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

	if (existingHash != hash || !cacheExists || appBuildChanged)
	{
		MakefileGenerator generator(m_state, m_project, m_toolchain);
		std::ofstream(m_cacheFile) << generator.getContents(inOutputs)
								   << std::endl;

		buildCache[key] = String::getPathFilename(m_cacheFile);
		m_state.cache.setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::initialize()
{
	const auto& makeExec = m_state.tools.make();
	// if (!Commands::pathExists(makeExec))
	// {
	// 	Diagnostic::error(fmt::format("{} was not found in compiler path. Aborting.", makeExec));
	// 	return false;
	// }

	const auto maxJobs = m_project.maxJobs();

	std::string jobs;
	if (maxJobs > 0)
		jobs = fmt::format(" -j{}", maxJobs);

	std::string makeParams = fmt::format("-C \".\" -f \"{}\" --no-print-directory", m_cacheFile);

	m_makeAsync = fmt::format("{makeExec}{jobs} {makeParams}",
		FMT_ARG(makeExec),
		FMT_ARG(jobs),
		FMT_ARG(makeParams));

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::run()
{
	// Timer timer;

	if (!Commands::shell(fmt::format("{} makebuild", m_makeAsync)))
	{
		Output::lineBreak();
		return false;
	}

	if (m_project.dumpAssembly())
	{
		if (!Commands::shell(fmt::format("{} dumpasm", m_makeAsync)))
		{
			Output::lineBreak();
			return false;
		}
	}

	// This is typically build time - 20 ms
	// auto result = timer.stop();
	// Output::print(Color::reset, fmt::format("   Make invocation time: {}ms", result));

	return true;
}

}
