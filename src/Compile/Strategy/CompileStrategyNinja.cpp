/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Compile/Strategy/NinjaGenerator.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyNinja::CompileStrategyNinja(BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
}

/*****************************************************************************/
StrategyType CompileStrategyNinja::type() const
{
	return StrategyType::Ninja;
}

/*****************************************************************************/
bool CompileStrategyNinja::createCache(const SourceOutputs& inOutputs)
{
	auto& name = m_project.name();
	std::string cacheFolder = m_state.cache.getHash(name, BuildCache::Type::Local);

	auto& environmentCache = m_state.cache.environmentCache();
	Json& buildCache = environmentCache.json["data"];
	std::string key = fmt::format("{}:{}", m_state.buildConfiguration(), name);

	std::string existingHash;
	if (buildCache.contains(key))
	{
		existingHash = buildCache.at(key);
	}

	m_cacheFile = fmt::format("{}/build.ninja", cacheFolder);
	const bool cacheExists = Commands::pathExists(cacheFolder) && Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(cacheFolder);

	if (existingHash != hash || !cacheExists || appBuildChanged)
	{
		if (!cacheExists)
		{
			Commands::makeDirectory(cacheFolder);
		}

		NinjaGenerator generator(m_state, m_project, m_toolchain);
		std::ofstream(m_cacheFile) << generator.getContents(inOutputs, cacheFolder)
								   << std::endl;

		buildCache[key] = String::getPathFilename(cacheFolder);
		m_state.cache.setDirty(true);
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::initialize()
{
	auto& ninjaExec = m_state.tools.ninja();
	if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
	{
		Diagnostic::error(fmt::format("{} was not found in compiler path. Aborting.", ninjaExec));
		return false;
	}

	m_ninjaCmd.clear();
	m_ninjaCmd.push_back(ninjaExec);
	m_ninjaCmd.push_back("-f");
	m_ninjaCmd.push_back(m_cacheFile);

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::run()
{
	{
		StringList command;
		std::cout << Output::getAnsiStyle(Color::Blue);

		bool result = Commands::subprocess(m_ninjaCmd, PipeOption::StdOut);
		Output::lineBreak();

		if (!result)
			return false;
	}

	if (m_project.dumpAssembly())
	{
		StringList command;
		std::cout << Output::getAnsiStyle(Color::Magenta);

		bool result = Commands::subprocess(m_ninjaCmd, PipeOption::StdOut);
		Output::lineBreak();

		if (!result)
			return false;
	}

	return true;
}

}
