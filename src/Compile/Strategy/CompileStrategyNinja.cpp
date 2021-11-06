/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/CompileStrategyNinja.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"

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

	const auto& uniqueId = m_state.uniqueId();

	auto& cacheFile = m_state.cache.file();
	m_cacheFolder = m_state.cache.getCachePath(uniqueId, CacheType::Local);
	m_cacheFile = fmt::format("{}/build.ninja", m_cacheFolder);

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
bool CompileStrategyNinja::addProject(const SourceTarget& inProject, SourceOutputs&& inOutputs, CompileToolchain& inToolchain)
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
bool CompileStrategyNinja::buildProject(const SourceTarget& inProject)
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

	static const char* kNinjaStatus = "NINJA_STATUS";
	auto oldNinjaStatus = Environment::getAsString(kNinjaStatus);

	auto color = Output::getAnsiStyle(Output::theme().build);
	Environment::set(kNinjaStatus, fmt::format("   [%f/%t] {}", color));

	command.emplace_back(fmt::format("build_{}", hash));
	bool result = subprocessNinja(command);

	Environment::set(kNinjaStatus, oldNinjaStatus);

	return result;
}

/*****************************************************************************/
bool CompileStrategyNinja::subprocessNinja(const StringList& inCmd, std::string inCwd) const
{
	// if (Output::showCommands())
	// 	Output::print(Output::theme().build, inCmd);

	std::string data;
	auto eol = String::eol();
	auto endlineReplace = fmt::format("{}\n", Output::getAnsiStyle(Color::Reset));

	ProcessOptions::PipeFunc onStdOut = [&](std::string inData) -> void {
		String::replaceAll(inData, eol, endlineReplace);
		std::cout << inData << std::flush;

		auto lineBreak = inData.find('\n');
		if (lineBreak == std::string::npos)
		{
			data += std::move(inData);
		}
		else
		{
			data += inData.substr(0, lineBreak + 1);
			auto tmp = inData.substr(lineBreak + 1);
			if (!tmp.empty())
			{
				data = std::move(tmp);
			}
		}
	};

	ProcessOptions options;
	options.cwd = std::move(inCwd);
	options.stdoutOption = PipeOption::Pipe;
	options.stderrOption = PipeOption::StdErr;
	options.onStdOut = std::move(onStdOut);

	int result = Process::run(inCmd, options);

#if defined(CHALET_WIN32)
	if (data.size() > 0)
	{
		std::string noWork = fmt::format("ninja: no work to do.{}", endlineReplace);
		if (String::endsWith(noWork, data))
			Output::previousLine();
		else
			Output::lineBreak();
	}
#endif

	return result == EXIT_SUCCESS;
}
}
