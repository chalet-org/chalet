/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Process/ProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
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
bool CompileStrategyMakefile::initialize()
{
	if (m_initialized)
		return false;

	const auto& uniqueId = m_state.uniqueId();
	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(uniqueId, CacheType::Local);

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
bool CompileStrategyMakefile::addProject(const SourceTarget& inProject)
{
	if (!m_initialized)
		return false;

	auto& name = inProject.name();
	const auto& outputs = m_outputs.at(name);
	if (m_hashes.find(name) == m_hashes.end())
	{
		m_hashes.emplace(name, Hash::string(outputs->target));
	}
	if (m_buildFiles.find(name) == m_buildFiles.end())
	{
		m_buildFiles.emplace(name, fmt::format("{}/{}.mk", m_cacheFolder, inProject.name()));
	}

	auto& buildFile = m_buildFiles.at(name);
	if (m_cacheNeedsUpdate || !Commands::pathExists(buildFile))
	{
		auto& hash = m_hashes.at(name);
		auto& toolchain = m_toolchains.at(name);
		m_generator->addProjectRecipes(inProject, *outputs, toolchain, hash);

		std::ofstream(buildFile) << m_generator->getContents(buildFile)
								 << std::endl;

		m_generator->reset();
	}

	auto& hash = m_hashes.at(inProject.name());
	setBuildEnvironment(*outputs, hash);

	return ICompileStrategy::addProject(inProject);
}

/*****************************************************************************/
bool CompileStrategyMakefile::doPreBuild()
{
	// if (m_initialized && m_generator->hasProjectRecipes()) {}

	return ICompileStrategy::doPreBuild();
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

#if defined(CHALET_WIN32)
	auto buildColor = Output::getAnsiStyle(Output::theme().build);
	std::cout.write(buildColor.data(), buildColor.size());
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
	auto buildColor = Output::getAnsiStyle(Output::theme().build);
	std::cout.write(buildColor.data(), buildColor.size());
	#endif

	command.emplace_back(std::string());

	if (inProject.usesPch())
	{
		command.back() = fmt::format("pch_{}", hash);

		bool result = subprocessMakefile(command);
		if (!result)
			return false;
	}

	{

		command.back() = fmt::format("build_{}", hash);

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
	// 	std::cerr.write(inData.data(), inData.size());
	// 	std::cerr.flush();
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
			std::cout.write(inData.data(), inData.size());
			std::cout.flush();
		};
	}
	else
	{
		// options.onStdOut = [](std::string inData) {
		// 	String::replaceAll(inData, "\r\n", "\n");
		// 	std::cout.write(inData.data(), inData.size());
		//  std::cout.flush();
		// };
	}

#endif

	int result = ProcessController::run(inCmd, options);
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
		auto reset = Output::getAnsiStyle(Color::Reset);
#if defined(CHALET_WIN32)
		if (result == EXIT_SUCCESS)
#endif
		{
			std::cout.write(reset.data(), reset.size());
			std::cout.write(errorOutput.data(), errorOutput.size());
			std::cout.put(std::cout.widen('\n'));
			std::cout.flush();
		}
#if defined(CHALET_WIN32)
		else
		{
			std::cout.write(reset.data(), reset.size());
			std::cout.write(errorOutput.data(), errorOutput.size());
			std::cout.flush();
		}
#endif
	}

	return result == EXIT_SUCCESS;
}

/*****************************************************************************/
void CompileStrategyMakefile::setBuildEnvironment(const SourceOutputs& inOutput, const std::string& inHash) const
{
	auto objects = String::join(inOutput.objectListLinker);
	Environment::set(fmt::format("OBJS_{}", inHash).c_str(), objects);

	StringList depends;
	for (auto& group : inOutput.groups)
	{
		depends.push_back(group->dependencyFile);
	}

	auto depdendencies = String::join(std::move(depends));
	Environment::set(fmt::format("DEPS_{}", inHash).c_str(), depdendencies);
}

/*****************************************************************************/
bool CompileStrategyMakefile::doPostBuild() const
{
	std::string emptyString;
	for (auto& [_, hash] : m_hashes)
	{
		Environment::set(fmt::format("OBJS_{}", hash).c_str(), emptyString);
		Environment::set(fmt::format("DEPS_{}", hash).c_str(), emptyString);
	}

	return ICompileStrategy::doPostBuild();
}
}
