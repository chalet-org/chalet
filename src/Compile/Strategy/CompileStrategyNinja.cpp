/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"
#include "Utility/Subprocess.hpp"

namespace chalet
{
/*****************************************************************************/
CompileStrategyNinja::CompileStrategyNinja(BuildState& inState) :
	ICompileStrategy(StrategyType::Ninja, inState)
{
}

/*****************************************************************************/
bool CompileStrategyNinja::initialize()
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
bool CompileStrategyNinja::addProject(const ProjectConfiguration& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
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
bool CompileStrategyNinja::saveBuildFile() const
{
	if (!m_initialized || !m_generator->hasProjectRecipes())
		return false;

	if (m_cacheNeedsUpdate)
	{
		std::ofstream(m_cacheFile) << m_generator->getContents(m_cacheFolder)
								   << std::endl;
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::buildProject(const ProjectConfiguration& inProject) const
{
	auto& ninjaExec = m_state.tools.ninja();
	if (ninjaExec.empty() || !Commands::pathExists(ninjaExec))
	{
		Diagnostic::error(fmt::format("{} was not found in compiler path. Aborting.", ninjaExec));
		return false;
	}

	if (m_hashes.find(inProject.name()) == m_hashes.end())
	{
		Diagnostic::error(fmt::format("{} was not previously cached. Aborting.", inProject.name()));
		return false;
	}

	StringList command;
	command.push_back(ninjaExec);

	if (m_state.environment.showCommands())
		command.push_back("-v");

	command.push_back("-f");
	command.push_back(m_cacheFile);

	if (m_state.tools.ninjaVersionMajor() >= 1 && m_state.tools.ninjaVersionMinor() > 10)
	{
		// silences ninja status updates
		command.push_back("--quiet"); // forthcoming (in ninja's master branch currently)
	}

	const bool clean = true;
	auto& hash = m_hashes.at(inProject.name());

	{
		std::cout << Output::getAnsiStyle(Color::Blue) << std::flush;

		command.push_back(fmt::format("build_{}", hash));
		bool result = subprocessNinja(command, clean);

		if (!result)
			return false;
	}

	if (inProject.dumpAssembly())
	{
		std::cout << Output::getAnsiStyle(Color::Magenta) << std::flush;

		command.back() = fmt::format("asm_{}", hash);

		bool result = subprocessNinja(command, clean);

		if (!result)
			return false;
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::subprocessNinja(const StringList& inCmd, const bool inCleanOutput, std::string inCwd) const
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

	bool skipOutput = false;
	std::string noWork{ "ninja: no work to do.\n" };
	Subprocess::PipeFunc onStdOut = [&skipOutput, &noWork](std::string inData) {
#if defined(CHALET_WIN32)
		String::replaceAll(inData, "\r\n", "\n");
#endif

#if defined(CHALET_WIN32)
		if (skipOutput || String::endsWith(noWork, inData) || String::equals('n', inData))
#else
		if (skipOutput || String::endsWith(noWork, inData))
#endif
		{
			skipOutput = true;
			return;
		}

		std::cout << inData << std::flush;
	};

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::Pipe;
	options.onStdErr = onStdErr;
	options.onStdOut = onStdOut;

	// std::size_t cutoff = std::string::npos;
	// cutoff = errorOutput.find("ninja: no work to do.");

	// if (cutoff != std::string::npos)
	// {
	// 	errorOutput = errorOutput.substr(0, cutoff);
	// }

	int result = Subprocess::run(inCmd, std::move(options));
	if (!errorOutput.empty())
	{
		// std::size_t cutoff = std::string::npos;

		// Note: std::cerr outputs after std::cout on windows (which we don't want)
		if (result == EXIT_SUCCESS)
			std::cout << Output::getAnsiReset() << errorOutput << std::endl;
		else
			std::cout << Output::getAnsiReset() << errorOutput << std::flush;
	}

	if (!skipOutput)
		Output::lineBreak();

	return result == EXIT_SUCCESS;
}
}
