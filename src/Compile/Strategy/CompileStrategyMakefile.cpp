/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyMakefile.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Process/Environment.hpp"
#include "Process/SubProcessController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyMakefile::CompileStrategyMakefile(BuildState& inState) :
	ICompileStrategy(StrategyType::Makefile, inState)
{
}

/*****************************************************************************/
std::string CompileStrategyMakefile::name() const noexcept
{
#if defined(CHALET_WIN32)
	if (m_state.toolchain.makeIsNMake())
	{
		if (m_state.toolchain.makeIsJom())
			return "NMAKE (Qt Jom)";
		else
			return "NMAKE";
	}
	else
#endif
	{
		return "GNU Make";
	}
}

/*****************************************************************************/
bool CompileStrategyMakefile::initialize()
{
	if (m_initialized)
		return false;

	const auto& cachePathId = m_state.cachePathId();

	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(cachePathId);

	const bool cacheExists = Files::pathExists(m_cacheFolder);
	const bool appVersionChanged = cacheFile.appVersionChanged();
	const bool themeChanged = cacheFile.themeChanged();
	const bool buildFileChanged = cacheFile.buildFileChanged();
	const bool buildHashChanged = cacheFile.buildHashChanged();

	m_cacheNeedsUpdate = !cacheExists || appVersionChanged || buildHashChanged || buildFileChanged || themeChanged;

	if (!Files::pathExists(m_cacheFolder))
		Files::makeDirectory(m_cacheFolder);

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
	if (m_cacheNeedsUpdate || !Files::pathExists(buildFile))
	{
		auto& hash = m_hashes.at(name);
		auto& toolchain = m_toolchains.at(name);
		m_generator->addProjectRecipes(inProject, *outputs, toolchain, hash);

		std::ofstream(buildFile) << m_generator->getContents(buildFile)
								 << std::endl;

		m_generator->reset();
	}

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

	bool result = false;
#if defined(CHALET_WIN32)
	const bool makeIsNMake = m_state.toolchain.makeIsNMake();
	if (makeIsNMake)
	{
		result = buildNMake(inProject);
	}
	else
#endif
	{
		result = buildMake(inProject);
	}

	if (result)
	{
		checkIfTargetWasUpdated(inProject);
		return ICompileStrategy::buildProject(inProject);
	}
	else
	{
		m_filesUpdated = true;
		return false;
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

		if (m_state.info.keepGoing())
			command.emplace_back("--keep-going");

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

		if (m_state.info.keepGoing())
			command.emplace_back("/K");

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

	command.emplace_back(std::string());

	if (inProject.usesPrecompiledHeader())
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
	// static ProcessOptions::PipeFunc onStdErr = [](std::string inData) {
	// 	std::cerr.write(inData.data(), inData.size());
	// 	std::cerr.flush();
	// };

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::StdOut;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = [&errorOutput](std::string inData) {
		errorOutput += std::move(inData);
	};

#if defined(CHALET_WIN32)
	// options.stdoutOption = PipeOption::Pipe;
	if (m_state.toolchain.makeIsNMake())
	{
		options.stdoutOption = PipeOption::Pipe;
		options.onStdOut = [](std::string inData) {
			String::replaceAll(inData, ": warning ", Output::getAnsiStyle(Output::theme().reset) + ": warning ");
			String::replaceAll(inData, ": error ", Output::getAnsiStyle(Output::theme().reset) + ": error ");
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

	i32 result = SubProcessController::run(inCmd, options);
	if (!errorOutput.empty())
	{
		size_t cutoff = std::string::npos;
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
			String::replaceAll(errorOutput, fmt::format("{}: *** Waiting for unfinished jobs....\n", make), "");
			String::replaceAll(errorOutput, fmt::format("{}: *** No rule", make), "No rule");
			cutoff = errorOutput.find(fmt::format("{}: *** [", make));
		}

		if (cutoff != std::string::npos)
		{
			errorOutput = errorOutput.substr(0, cutoff);
		}

		// Note: std::cerr outputs after std::cout on windows (which we don't want)
		const auto& reset = Output::getAnsiStyle(Output::theme().reset);
#if defined(CHALET_WIN32)
		if (result == EXIT_SUCCESS)
#endif
		{
			std::cout.write(reset.data(), reset.size());
			std::cout.write(errorOutput.data(), errorOutput.size());
			std::cout.put('\n');
			std::cout.flush();
		}
#if defined(CHALET_WIN32)
		else
		{
			std::cout.write(reset.data(), reset.size());
			std::cout.write(errorOutput.data(), errorOutput.size());
			std::cout.flush();

			if (m_state.toolchain.makeIsNMake())
				Output::lineBreak();
		}
#endif
	}

	return result == EXIT_SUCCESS;
}

}
