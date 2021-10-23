/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
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
bool CompileStrategyMakefile::initialize(const StringList& inFileExtensions)
{
	if (m_initialized)
		return false;

	const auto uniqueId = m_state.getUniqueIdForState(inFileExtensions);

	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(uniqueId, CacheType::Local);

	cacheFile.setSourceCache(uniqueId);

	const bool cacheExists = Commands::pathExists(m_cacheFolder);
	const bool appVersionChanged = cacheFile.appVersionChanged();
	const bool themeChanged = cacheFile.themeChanged();
	const bool buildFileChanged = cacheFile.buildFileChanged();
	const bool buildHashChanged = cacheFile.buildHashChanged();

	m_cacheNeedsUpdate = !cacheExists || appVersionChanged || buildHashChanged || buildFileChanged || themeChanged;

	if (!Commands::pathExists(m_cacheFolder))
		Commands::makeDirectory(m_cacheFolder);

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
{
	if (!m_initialized)
		return false;

	auto& name = inProject.name();
	if (m_hashes.find(name) == m_hashes.end())
	{
		m_hashes.emplace(name, Hash::string(inOutputs.target));
	}
	if (m_buildFiles.find(name) == m_buildFiles.end())
	{
		m_buildFiles.emplace(name, fmt::format("{}/{}.mk", m_cacheFolder, inProject.name()));
	}

	auto& buildFile = m_buildFiles.at(name);
	if (m_cacheNeedsUpdate || !Commands::pathExists(buildFile))
	{
		auto& hash = m_hashes.at(name);
		m_generator->addProjectRecipes(inProject, inOutputs, inToolchain, hash);

		std::ofstream(buildFile) << m_generator->getContents(buildFile)
								 << std::endl;

		m_generator->reset();
	}

	m_outputs[name] = std::move(inOutputs);

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::saveBuildFile() const
{
	if (!m_initialized || !m_generator->hasProjectRecipes())
		return false;

	return true;
}

/*****************************************************************************/
bool CompileStrategyMakefile::buildProject(const SourceTarget& inProject)
{
	if (m_hashes.find(inProject.name()) == m_hashes.end())
		return false;

	StringList command;

#if defined(CHALET_WIN32)
	const bool makeIsNMake = m_state.toolchain.makeIsNMake();
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
bool CompileStrategyMakefile::buildMake(const SourceTarget& inProject) const
{
	const auto& makeExec = m_state.toolchain.make();
	StringList command;

	auto& buildFile = m_buildFiles.at(inProject.name());
	{
		std::string jobs;
		const auto maxJobs = m_state.info.maxJobs();
		if (maxJobs > 0)
			jobs = fmt::format("-j{}", maxJobs);

		command.clear();
		command.push_back(makeExec);
		if (!jobs.empty())
		{
			command.push_back(jobs);
		}
		// command.emplace_back("-d");
		command.emplace_back("-C");
		command.emplace_back(".");
		command.emplace_back("-f");
		command.push_back(buildFile);

		/*if (m_state.toolchain.makeVersionMajor() >= 4)
		{
			command.emplace_back("--output-sync=target");
		}*/

		command.emplace_back("--no-builtin-rules");
		command.emplace_back("--no-builtin-variables");
		command.emplace_back("--no-print-directory");
	}

	auto& hash = m_hashes.at(inProject.name());

	auto& outputs = m_outputs.at(inProject.name());
	m_state.paths.setBuildEnvironment(outputs, hash);

#if defined(CHALET_WIN32)
	std::cout << Output::getAnsiStyle(Output::theme().build);
#endif

	{
		command.emplace_back(fmt::format("build_{}", hash));
		bool result = subprocessMakefile(command);
		if (!result)
		{
#if defined(CHALET_WIN32)
			Output::lineBreak();
#endif
			return false;
		}
	}

	return true;
}

#if defined(CHALET_WIN32)
/*****************************************************************************/
bool CompileStrategyMakefile::buildNMake(const SourceTarget& inProject) const
{
	const auto& makeExec = m_state.toolchain.make();

	StringList command;

	auto& buildFile = m_buildFiles.at(inProject.name());

	const bool makeIsNMake = m_state.toolchain.makeIsNMake();
	const bool makeIsJom = m_state.toolchain.makeIsJom();
	if (makeIsNMake)
	{
		command.clear();
		command.push_back(makeExec);
		command.emplace_back("/NOLOGO");
		command.emplace_back("/F");
		command.push_back(buildFile);
	}

	if (makeIsJom)
	{
		const auto maxJobs = m_state.info.maxJobs();

		command.emplace_back("/J" + std::to_string(maxJobs));
		// command.emplace_back(std::to_string(maxJobs));
	}

	auto& hash = m_hashes.at(inProject.name());

	#if defined(CHALET_WIN32)
	std::cout << Output::getAnsiStyle(Output::theme().build);
	#endif

	command.emplace_back(std::string());

	if (inProject.usesPch())
	{
		command.back() = fmt::format("pch_{}", hash);
		Environment::set("CL", "");

		bool result = subprocessMakefile(command);
		if (!result)
			return false;
	}

	{

		command.back() = fmt::format("build_{}", hash);
		Environment::set("CL", "/MP"); // doesn't work

		bool result = subprocessMakefile(command);
		if (!result)
			return false;
	}

	return true;
}
#endif

/*****************************************************************************/
bool CompileStrategyMakefile::subprocessMakefile(const StringList& inCmd, std::string inCwd) const
{
	// if (Output::showCommands())
	// 	Output::print(Output::theme().build, inCmd);

	std::string errorOutput;
	ProcessOptions::PipeFunc onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};
	// static ProcessOptions::PipeFunc onStdErr = [](std::string inData) {
	// 	std::cerr << inData << std::flush;
	// };

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = std::move(onStdErr);

#if defined(CHALET_WIN32)
	// options.stdoutOption = PipeOption::Pipe;
	if (m_state.toolchain.makeIsNMake())
	{
		options.stdoutOption = PipeOption::Pipe;
		options.onStdOut = [](std::string inData) {
			String::replaceAll(inData, "\r\n", "\n");
			String::replaceAll(inData, ": warning ", Output::getAnsiStyle(Color::Reset) + ": warning ");
			String::replaceAll(inData, ": error ", Output::getAnsiStyle(Color::Reset) + ": error ");
			std::cout << std::move(inData) << std::flush;
		};
	}
	else
	{
		// options.onStdOut = [](std::string inData) {
		// 	String::replaceAll(inData, "\r\n", "\n");
		// 	std::cout << inData << std::flush;
		// };
	}

#endif

	int result = Process::run(inCmd, options);
	if (!errorOutput.empty())
	{
		std::size_t cutoff = std::string::npos;
		const auto make = String::getPathBaseName(m_state.toolchain.make());

#if defined(CHALET_WIN32)
		if (m_state.toolchain.makeIsNMake())
		{
			String::replaceAll(errorOutput, '\r', "\r\n");

			if (m_state.toolchain.makeIsJom())
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
#if defined(CHALET_WIN32)
			String::replaceAll(errorOutput, "\r\n", "\n");
#endif
			String::replaceAll(errorOutput, fmt::format("{}: *** Waiting for unfinished jobs....\n", make), "");
			String::replaceAll(errorOutput, fmt::format("{}: *** No rule", make), "No rule");
			cutoff = errorOutput.find(fmt::format("{}: *** [", make));
		}

		if (cutoff != std::string::npos)
		{
			errorOutput = errorOutput.substr(0, cutoff);
		}

		// Note: std::cerr outputs after std::cout on windows (which we don't want)
#if defined(CHALET_WIN32)
		if (result == EXIT_SUCCESS)
#endif
			std::cout << Output::getAnsiStyle(Color::Reset) << errorOutput << std::endl;
#if defined(CHALET_WIN32)
		else
			std::cout << Output::getAnsiStyle(Color::Reset) << errorOutput << std::flush;
#endif
	}

	return result == EXIT_SUCCESS;
}
}
