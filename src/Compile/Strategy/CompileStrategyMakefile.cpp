/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

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
	const auto key = m_state.cache.getCacheKey(name);

	const bool cacheExists = Commands::pathExists(m_cacheFile);
	const bool appBuildChanged = m_state.cache.appBuildChanged();
	const auto hash = String::getPathFilename(m_cacheFile);

	std::string existingHash;
	if (buildCache.contains(key))
	{
		existingHash = buildCache.at(key);
	}

	m_cacheNeedsUpdate = existingHash != hash || !cacheExists || appBuildChanged;

	if (m_cacheNeedsUpdate)
	{
		buildCache[key] = hash;
		m_state.cache.setDirty(true);
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	if (!m_initialized)
		return false;

	auto& name = inProject.name();
	if (m_hashes.find(name) == m_hashes.end())
	{
		m_hashes.emplace(name, Hash::string(inOutputs.target));
	}

	if (m_cacheNeedsUpdate)
	{
		auto& hash = m_hashes.at(name);
		m_generator->addProjectRecipes(inProject, inOutputs, inToolchain, hash);
	}

	m_outputs[name] = std::move(inOutputs);

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
bool CompileStrategyMakefile::buildProject(const ProjectTarget& inProject) const
{
	if (m_hashes.find(inProject.name()) == m_hashes.end())
		return false;

	StringList command;

#if defined(CHALET_WIN32)
	const bool makeIsNMake = m_state.tools.makeIsNMake();
	if (makeIsNMake)
	{
		return buildNMake(inProject);
	}
	else
#endif
	{
		return buildMake(inProject);
	}
}

/*****************************************************************************/
bool CompileStrategyMakefile::buildMake(const ProjectTarget& inProject) const
{
	const auto& makeExec = m_state.tools.make();
	StringList command;

	{
		std::string jobs;
		const auto maxJobs = m_state.environment.maxJobs();
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

	auto& outputs = m_outputs.at(inProject.name());
	m_state.paths.setBuildEnvironment(outputs, hash, inProject.dumpAssembly());

	const bool clean = true;
#if defined(CHALET_WIN32)
	std::cout << Output::getAnsiStyle(Color::Blue);
#endif

	{

		command.push_back(fmt::format("build_{}", hash));
		bool result = subprocessMakefile(command, clean);

		if (!result)
			return false;
	}

	if (inProject.dumpAssembly())
	{
		command.back() = fmt::format("asm_{}", hash);
		bool result = subprocessMakefile(command, clean);

		if (!result)
			return false;
	}

	return true;
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool CompileStrategyMakefile::buildNMake(const ProjectTarget& inProject) const
{
	const auto& makeExec = m_state.tools.make();

	StringList command;

	const bool makeIsNMake = m_state.tools.makeIsNMake();
	const bool makeIsJom = m_state.tools.makeIsJom();
	if (makeIsNMake)
	{
		command.clear();
		command.push_back(makeExec);
		command.push_back("/NOLOGO");
		command.push_back("/F");
		command.push_back(m_cacheFile);
	}

	if (makeIsJom)
	{
		const auto maxJobs = m_state.environment.maxJobs();

		command.push_back("/J" + std::to_string(maxJobs));
		// command.push_back(std::to_string(maxJobs));
	}

	auto& hash = m_hashes.at(inProject.name());

	const bool clean = true;
	#if defined(CHALET_WIN32)
	std::cout << Output::getAnsiStyle(Color::Blue);
	#endif

	command.push_back(std::string());

	if (inProject.usesPch())
	{
		command.back() = fmt::format("pch_{}", hash);
		Environment::set("CL", "");

		bool result = subprocessMakefile(command, clean);
		if (!result)
			return false;
	}

	{

		command.back() = fmt::format("build_{}", hash);
		Environment::set("CL", "/MP"); // doesn't work

		bool result = subprocessMakefile(command, clean);
		if (!result)
			return false;
	}

	if (inProject.dumpAssembly())
	{
		command.back() = fmt::format("asm_{}", hash);

		bool result = subprocessMakefile(command, clean);
		if (!result)
			return false;
	}

	return true;
}
#endif

/*****************************************************************************/
bool CompileStrategyMakefile::subprocessMakefile(const StringList& inCmd, const bool inCleanOutput, std::string inCwd) const
{
	if (!inCleanOutput)
		Output::print(Color::Blue, inCmd);

	std::string errorOutput;
	Subprocess::PipeFunc onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};
	// static Subprocess::PipeFunc onStdErr = [](std::string inData) {
	// 	std::cerr << inData << std::flush;
	// };

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = onStdErr;

#if defined(CHALET_WIN32)
	options.stdoutOption = PipeOption::Pipe;
	if (m_state.tools.makeIsNMake())
	{
		options.onStdOut = [](std::string inData) {
			String::replaceAll(inData, "\r\n", "\n");
			String::replaceAll(inData, ": warning ", Output::getAnsiReset() + ": warning ");
			String::replaceAll(inData, ": error ", Output::getAnsiReset() + ": error ");
			std::cout << inData << std::flush;
		};
	}
	else
	{
		options.onStdOut = [](std::string inData) {
			String::replaceAll(inData, "\r\n", "\n");
			std::cout << inData << std::flush;
		};
	}

#endif

	int result = Subprocess::run(inCmd, std::move(options));
	if (!errorOutput.empty())
	{
		std::size_t cutoff = std::string::npos;
		const auto make = String::getPathBaseName(m_state.tools.make());

#if defined(CHALET_WIN32)
		if (m_state.tools.makeIsNMake())
		{
			String::replaceAll(errorOutput, "\r", "\r\n");

			if (m_state.tools.makeIsJom())
			{
				cutoff = errorOutput.find("jom: ");
			}
			else
			{
				cutoff = errorOutput.find("NMAKE : fatal error");
			}
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
