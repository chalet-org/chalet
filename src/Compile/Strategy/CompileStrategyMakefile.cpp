/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Compile/Strategy/MakefileGenerator.hpp"
#include "Compile/Strategy/MakefileGeneratorNMake.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"
#include "Utility/Timer.hpp"

// #define CHALET_KEEP_OLD_CACHE 1

namespace chalet
{
/*****************************************************************************/
namespace
{
/*****************************************************************************/
void signalHandler(int inSignal)
{
	Subprocess::haltAllProcesses(inSignal);
}
}

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

#if defined(CHALET_KEEP_OLD_CACHE)
	UNUSED(m_toolchain);
#else
	const bool cacheExists = Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(m_cacheFile);

	if (existingHash != hash || !cacheExists || appBuildChanged)
	{
	#if defined(CHALET_WIN32)
		const bool isNMake = m_state.tools.isNMake();
		if (isNMake)
		{
			MakefileGeneratorNMake generator(m_state, m_project, m_toolchain);
			std::ofstream(m_cacheFile) << generator.getContents(inOutputs)
									   << std::endl;
		}
		else
	#endif
		{
			MakefileGenerator generator(m_state, m_project, m_toolchain);
			std::ofstream(m_cacheFile) << generator.getContents(inOutputs)
									   << std::endl;
		}

		buildCache[key] = String::getPathFilename(m_cacheFile);
		m_state.cache.setDirty(true);
	}
#endif

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::initialize()
{
	const auto& makeExec = m_state.tools.make();
	const auto maxJobs = m_state.environment.maxJobs();

#if defined(CHALET_WIN32)
	const bool isNMake = m_state.tools.isNMake();
	if (isNMake)
	{
		m_makeCmd.clear();
		m_makeCmd.push_back(makeExec);
		m_makeCmd.push_back("/NOLOGO");
		m_makeCmd.push_back("/F");
		m_makeCmd.push_back(m_cacheFile);
	}
	else
#endif
	{
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
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::run()
{
	::signal(SIGINT, signalHandler);
	::signal(SIGTERM, signalHandler);
	::signal(SIGABRT, signalHandler);

	// Timer timer;
	const bool clean = true;
#if defined(CHALET_WIN32)
	std::cout << Output::getAnsiStyle(Color::Blue);
#endif

	/*if (m_toolchain->type() == ToolchainType::MSVC)
	{
		LOG("Remove me when CompileToolchainMSVC is done!");
		return false;
	}*/

	// Note: If using subprocess, there's some weird color issues that show on MinGW & bash

	m_makeCmd.push_back("makebuild");
	if (!subprocessMakefile(m_makeCmd, clean))
	{
		Output::lineBreak();
		return false;
	}

	if (m_project.dumpAssembly())
	{
		m_makeCmd.pop_back();
		m_makeCmd.push_back("dumpasm");
		if (!subprocessMakefile(m_makeCmd, clean))
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
		Output::print(Color::Blue, inCmd);

	std::string errorOutput;
	Subprocess::PipeFunc onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = onStdErr;

#if defined(CHALET_WIN32)
	static Subprocess::PipeFunc onStdOut = [](std::string inData) {
		String::replaceAll(inData, "\r\n", "\n");
		std::cout << inData;
	};

	// NMAKE
	// if (Environment::isMsvc())
	{
		options.stdoutOption = PipeOption::Pipe;
		options.onStdOut = onStdOut;
	}
#endif

	int result = Subprocess::run(inCmd, std::move(options));
	if (!errorOutput.empty())
	{
		std::size_t cutoff = 0;
		const auto make = String::getPathBaseName(m_state.tools.make());

#if defined(CHALET_WIN32)
		if (Environment::isMsvc())
		{
			String::replaceAll(errorOutput, "\r", "\r\n");

			// const char eol = '\r\n';
			cutoff = errorOutput.find("NMAKE : fatal error");
		}
		else
#endif
		{
			const char eol = '\n';
			String::replaceAll(errorOutput, fmt::format("{}: *** Waiting for unfinished jobs....{}", make, eol), "");
			String::replaceAll(errorOutput, fmt::format("{}: *** No rule", make), "No rule");
			cutoff = errorOutput.find(fmt::format("{}: *** [", make));
		}

		if (cutoff != std::string::npos)
		{
			errorOutput = errorOutput.substr(0, cutoff);
		}

		// Note: std::cerr outputs after std::cout on windows (which we don't want)
		if (result == EXIT_SUCCESS)
			std::cout << Output::getAnsiReset() << errorOutput << std::endl;
		else
			std::cout << Output::getAnsiReset() << errorOutput << std::flush;
	}

	return result == EXIT_SUCCESS;
}

}
