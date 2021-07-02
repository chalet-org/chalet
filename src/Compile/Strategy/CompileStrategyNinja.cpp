/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "State/AncillaryTools.hpp"
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
bool CompileStrategyNinja::initialize(const StringList& inFileExtensions)
{
	if (m_initialized)
		return false;

	auto id = fmt::format("ninja_{}_{}", Output::showCommands() ? 1 : 0, String::join(inFileExtensions));
	m_cacheFile = m_state.cache.getHashPath(id, CacheType::Local);

	auto& cacheFile = m_state.cache.file();
	const auto& oldStrategyHash = cacheFile.hashStrategy();

	// Note: The ninja cache folder must not change between chalet.json changes
	{
		m_cacheFolder = m_state.cache.getHashPath(m_state.paths.configuration(), CacheType::Local);
		cacheFile.addExtraHash(String::getPathFilename(m_cacheFolder));
	}

	const bool cacheExists = Commands::pathExists(m_cacheFolder) && Commands::pathExists(m_cacheFile);
	const bool appVersionChanged = cacheFile.appVersionChanged();
	const bool themeChanged = cacheFile.themeChanged();
	const bool buildFileChanged = cacheFile.buildFileChanged();

	auto strategyHash = String::getPathFilename(m_cacheFile);
	cacheFile.setSourceCache(strategyHash);

	m_cacheNeedsUpdate = oldStrategyHash != strategyHash || !cacheExists || appVersionChanged || themeChanged || buildFileChanged;

	if (m_cacheNeedsUpdate)
	{
		m_state.cache.file().setHashStrategy(std::move(strategyHash));
	}

	if (!Commands::pathExists(m_cacheFolder))
		Commands::makeDirectory(m_cacheFolder);

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::addProject(const ProjectTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
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
bool CompileStrategyNinja::buildProject(const ProjectTarget& inProject) const
{
	auto& ninjaExec = m_state.toolchain.ninja();
	if (m_hashes.find(inProject.name()) == m_hashes.end())
		return false;

	StringList command;
	command.push_back(ninjaExec);

	if (Output::showCommands())
		command.emplace_back("-v");

	command.emplace_back("-f");
	command.push_back(m_cacheFile);

	if (m_state.toolchain.ninjaVersionMajor() >= 1 && m_state.toolchain.ninjaVersionMinor() >= 10 && m_state.toolchain.ninjaVersionPatch() > 2)
	{
		// silences ninja status updates
		command.emplace_back("--quiet"); // forthcoming (in ninja's master branch currently)
	}

	auto& hash = m_hashes.at(inProject.name());

	{
		std::cout << Output::getAnsiStyle(Output::theme().build) << std::flush;

		command.emplace_back(fmt::format("build_{}", hash));
		bool result = subprocessNinja(command);

		if (!result)
			return false;
	}

	return true;
}

/*****************************************************************************/
bool CompileStrategyNinja::subprocessNinja(const StringList& inCmd, std::string inCwd) const
{
	// if (Output::showCommands())
	// 	Output::print(Output::theme().build, inCmd);

	bool skipOutput = false;
	std::string noWork{ "ninja: no work to do.\n" };
	Subprocess::PipeFunc onStdOut = [](std::string inData) {
#if defined(CHALET_WIN32)
		String::replaceAll(inData, "\r\n", "\n");
#endif

		/*
#if defined(CHALET_WIN32)
		// the n & inja split is a weird MinGW thing, although it seems consistent?
		if (skipOutput || String::endsWith(noWork, inData) || String::equals('n', inData))
#else
		if (skipOutput || String::endsWith(noWork, inData))
#endif
		{
			skipOutput = true;
			return;
		}
		*/

		std::cout << inData << std::flush;
	};

	SubprocessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = std::move(onStdOut);

	int result = Subprocess::run(inCmd, std::move(options));

	if (!skipOutput)
		Output::lineBreak();

	return result == EXIT_SUCCESS;
}
}
