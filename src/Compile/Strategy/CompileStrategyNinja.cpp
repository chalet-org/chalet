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

	const auto uniqueId = m_state.getUniqueIdForState(inFileExtensions);

	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(uniqueId, CacheType::Local);
	m_cacheFile = fmt::format("{}/build.ninja", m_cacheFolder);

	cacheFile.setSourceCache(uniqueId);

	const bool cacheExists = Commands::pathExists(m_cacheFolder) && Commands::pathExists(m_cacheFile);
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

	command.emplace_back("-d");
	command.emplace_back("keepdepfile");

	auto& hash = m_hashes.at(inProject.name());

	{
		// std::cout << Output::getAnsiStyle(Output::theme().build) << std::flush;

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
	std::string noWork{ "ninja: no work to do." };
	std::string data;
	auto eol = String::eol();
	auto endlineReplace = fmt::format("\n{}", Output::getAnsiReset());

	auto parsePrintOutput = [&]() {
		bool canSkip = data.size() > 5 && (String::startsWith(noWork, data) || String::startsWith(data, noWork));
		if (skipOutput || canSkip)
		{
			skipOutput = true;
			return;
		}

		if (data.front() == '[')
		{
			data.replace(0, 1, fmt::format("   {}[", Output::getAnsiReset()));

			auto firstEndBracket = data.find(']');
			if (firstEndBracket != std::string::npos)
			{
				if (data.substr(0, firstEndBracket).find('\n') == std::string::npos)
				{
					auto color = Output::getAnsiStyle(Output::theme().build);
					data.replace(firstEndBracket, 1, fmt::format("]{}", color));
				}
			}
		}

		String::replaceAll(data, eol, endlineReplace);

		std::cout << data << std::flush;
		data.clear();
	};

	ProcessOptions::PipeFunc onStdOut = [&data, &parsePrintOutput](std::string inData) {
#if defined(CHALET_WIN32)
		if (inData.size() == 1)
		{
			data += inData;
			return;
		}
		else
#endif
		{
			data += inData;
		}

		parsePrintOutput();
	};

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = std::move(onStdOut);

	int result = Subprocess::run(inCmd, std::move(options));

#if defined(CHALET_WIN32)
	if (data.size() > 0)
	{
		parsePrintOutput();
	}
#endif

	if (!skipOutput)
		Output::lineBreak();

	return result == EXIT_SUCCESS;
}
}
