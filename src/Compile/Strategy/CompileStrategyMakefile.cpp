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
#include "Utility/Subprocess.hpp"
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

	const auto maxJobs = m_state.environment.maxJobs();

	std::string jobs;
	if (maxJobs > 0)
		jobs = fmt::format("-j{}", maxJobs);

	m_makeCmd.clear();
	m_makeCmd.push_back(makeExec);
	if (!jobs.empty())
	{
		m_makeCmd.push_back(jobs);
	}
	m_makeCmd.push_back("-C");
	m_makeCmd.push_back(".");
	m_makeCmd.push_back("-f");
	m_makeCmd.push_back(m_cacheFile);
	m_makeCmd.push_back("--no-print-directory");

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::run()
{
	// Timer timer;
	const bool clean = true;
	std::cout << Output::getAnsiStyle(Color::Blue);

	// Note: If using subprocess, there's some weird color issues that show on MinGW & bash

	m_makeCmd.push_back("makebuild");
	// #if defined(CHALET_WIN32)
	// 	if (!Commands::shell(String::join(m_makeCmd), clean))
	// #else
	if (!subprocessMakefile(m_makeCmd, clean))
	// #endif
	{
		Output::lineBreak();
		return false;
	}

	if (m_project.dumpAssembly())
	{
		m_makeCmd.pop_back();
		m_makeCmd.push_back("dumpasm");
		// #if defined(CHALET_WIN32)
		// 		if (!Commands::shell(String::join(m_makeCmd), clean))
		// #else
		if (!subprocessMakefile(m_makeCmd, clean))
		// #endif
		{
			Output::lineBreak();
			return false;
		}
	}

	// This is typically build time - 20 ms
	// auto result = timer.stop();
	// Output::print(Color::Reset, fmt::format("   Make invocation time: {}ms", result));

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::subprocessMakefile(const StringList& inCmd, const bool inCleanOutput, std::string inCwd)
{
	if (!inCleanOutput)
	{
		std::cout << "Subprocess: ";
		Output::print(Color::Blue, inCmd);
	}

	std::string errorOutput;
	static Subprocess::PipeFunc onStderr = [&errorOutput](const std::string& inData) {
		errorOutput += inData;
	};

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = sp::PipeOption::cout;
	options.stderrOption = sp::PipeOption::pipe;
	options.onStderr = onStderr;

	int result = Subprocess::run(inCmd, options);
	if (!errorOutput.empty())
	{
		std::size_t cutoff = 0;
#if defined(CHALET_WIN32)
		if (String::contains("mingw32-make", m_state.tools.make()))
		{
			cutoff = errorOutput.find("mingw32-make: ***");
		}
		else
#endif
		{
			cutoff = errorOutput.find("make: ***");
		}
		if (cutoff != std::string::npos)
		{
			errorOutput = errorOutput.substr(0, cutoff);
		}

		// Note: std::cerr outputs after std::cout on windows (which we don't want)
		if (result == EXIT_SUCCESS)
			std::cout << errorOutput << std::endl;
		else
			std::cout << errorOutput << std::flush;
	}

	return result == EXIT_SUCCESS;
}

}
