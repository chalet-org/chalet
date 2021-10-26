/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerConfig.hpp"

#include "Cache/WorkspaceCache.hpp"
#include "Compile/CompilerConfigController.hpp"
#include "Compile/CompilerPathStructure.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/DefinesExperimental.hpp"
#include "Utility/Hash.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerConfig::CompilerConfig(const CodeLanguage inLanguage, const BuildState& inState, ICompileEnvironment& inEnvironment) :
	m_state(inState),
	m_environment(inEnvironment),
	m_language(inLanguage)
{
}

/*****************************************************************************/
CodeLanguage CompilerConfig::language() const noexcept
{
	return m_language;
}

/*****************************************************************************/
bool CompilerConfig::isInitialized() const noexcept
{
	return m_language != CodeLanguage::None;
}

/*****************************************************************************/
bool CompilerConfig::getSupportedCompilerFlags()
{
	if (m_environment.type() == ToolchainType::Unknown)
		return false;

	const auto& exec = m_state.toolchain.compilerCxx(m_language).path;
	if (exec.empty())
		return false;

	std::string flagsFile = m_state.cache.getHashPath(fmt::format("flags_{}.env", exec), CacheType::Local);

	m_state.cache.file().addExtraHash(String::getPathFilename(flagsFile));

	if (!Commands::pathExists(flagsFile))
	{
		if (m_state.compilers.isIntelClassic())
		{
			StringList categories{
				"codegen",
				"compatibility",
				"advanced",
				"component",
				"data",
				"diagnostics",
				"float",
				"inline",
				"ipo",
				"language",
				"link",
				"misc",
				"opt",
				"output",
				"pgo",
				"preproc",
				"reports",
				"openmp",
			};
			StringList cmd{ exec, "-Q" };
			std::string help{ "--help" };
			for (auto& category : categories)
			{
				cmd.push_back(help);
				cmd.emplace_back(category);
			}
			parseGnuHelpList(cmd);
		}
		else if (m_state.compilers.isGcc())
		{
			{
				StringList categories{
					"common",
					"optimizers",
					//"params",
					"target",
					"warnings",
					"undocumented",
				};
				StringList cmd{ exec, "-Q" };
				for (auto& category : categories)
				{
					cmd.emplace_back(fmt::format("--help={}", category));
				}
				parseGnuHelpList(cmd);
			}
			{
				StringList cmd{ exec, "-Wl,--help" };
				parseGnuHelpList(cmd);
			}

			// TODO: separate/joined -- kind of weird to check for
		}
		else if (m_state.compilers.isClang())
		{
			parseClangHelpList();
		}

		std::string outContents;
		for (auto& [flag, _] : m_supportedFlags)
		{
			outContents += flag + "\n";
		}

		std::ofstream(flagsFile) << outContents;
	}
	else
	{
		std::ifstream input(flagsFile);
		for (std::string line; std::getline(input, line);)
		{
			m_supportedFlags[std::move(line)] = true;
		}
	}

	return true;
}

/*****************************************************************************/
void CompilerConfig::parseGnuHelpList(const StringList& inCommand)
{
	auto path = String::getPathFolder(inCommand.front());
	std::string raw = Commands::subprocessOutput(inCommand, std::move(path));
	auto split = String::split(raw, '\n');

	for (auto& line : split)
	{
		auto beg = line.find_first_not_of(' ');
		auto end = line.find_first_of('=', beg);
		if (end == std::string::npos)
		{
			end = line.find_first_of('<', beg);
			if (end == std::string::npos)
			{
				end = line.find_first_of(' ', beg);
			}
		}

		if (beg != std::string::npos && end != std::string::npos)
		{
			line = line.substr(beg, end - beg);
		}

		if (String::startsWith('-', line))
		{
			if (String::contains('\t', line))
			{
				auto afterTab = line.find_last_of('\t');
				if (afterTab != std::string::npos)
				{
					std::string secondFlag = line.substr(afterTab);

					if (String::startsWith('-', secondFlag))
					{
						if (m_supportedFlags.find(secondFlag) == m_supportedFlags.end())
							m_supportedFlags.emplace(String::toLowerCase(secondFlag), true);
					}
				}

				end = line.find_first_of('"');
				if (end == std::string::npos)
				{
					end = line.find_first_of(' ');
				}

				line = line.substr(beg, end - beg);

				if (String::startsWith('-', line))
				{
					if (m_supportedFlags.find(line) == m_supportedFlags.end())
						m_supportedFlags.emplace(String::toLowerCase(line), true);
				}
			}
			else
			{
				if (m_supportedFlags.find(line) == m_supportedFlags.end())
					m_supportedFlags.emplace(String::toLowerCase(line), true);
			}
		}
	}
}

/*****************************************************************************/
void CompilerConfig::parseClangHelpList()
{
	const auto& exec = m_state.toolchain.compilerCxx(m_language).path;

	std::string raw = Commands::subprocessOutput({ exec, "-cc1", "--help" });
	auto split = String::split(raw, '\n');

	for (auto& line : split)
	{
		auto beg = line.find_first_not_of(' ');
		auto end = line.find_first_of('=', beg);
		if (end == std::string::npos)
		{
			end = line.find_first_of('<', beg);
			if (end == std::string::npos)
			{
				end = line.find_first_of(' ', beg);
			}
		}

		if (beg != std::string::npos && end != std::string::npos)
		{
			line = line.substr(beg, end - beg);
		}

		if (line.back() == ' ')
			line.pop_back();
		if (line.back() == ',')
			line.pop_back();

		if (String::startsWith('-', line))
		{
			if (String::contains('\t', line))
			{
				auto afterTab = line.find_last_of('\t');
				if (afterTab != std::string::npos)
				{
					std::string secondFlag = line.substr(afterTab);

					if (String::startsWith('-', secondFlag))
					{
						if (secondFlag.back() == ' ')
							secondFlag.pop_back();
						if (secondFlag.back() == ',')
							secondFlag.pop_back();

						if (m_supportedFlags.find(secondFlag) == m_supportedFlags.end())
							m_supportedFlags.emplace(String::toLowerCase(secondFlag), true);
					}
				}

				end = line.find_first_of('"');
				if (end == std::string::npos)
				{
					end = line.find_first_of(' ');
				}

				line = line.substr(beg, end - beg);

				if (String::startsWith('-', line))
				{
					if (line.back() == ' ')
						line.pop_back();
					if (line.back() == ',')
						line.pop_back();

					if (m_supportedFlags.find(line) == m_supportedFlags.end())
						m_supportedFlags.emplace(String::toLowerCase(line), true);
				}
			}
			else
			{
				if (m_supportedFlags.find(line) == m_supportedFlags.end())
					m_supportedFlags.emplace(String::toLowerCase(line), true);
			}
		}
	}

	// std::string supported;
	// for (auto& [flag, _] : m_supportedFlags)
	// {
	// 	supported += flag + '\n';
	// }
	// std::ofstream("clang_flags.txt") << supported;
}

/*****************************************************************************/
bool CompilerConfig::isFlagSupported(const std::string& inFlag) const
{
	return m_supportedFlags.find(inFlag) != m_supportedFlags.end();
}

/*****************************************************************************/
bool CompilerConfig::isLinkSupported(const std::string& inLink, const StringList& inDirectories) const
{
	const auto& exec = m_state.toolchain.compilerCxx(m_language).path;
	if (exec.empty())
		return false;

	if (m_state.compilers.isGcc())
	{
		// This will print the input if the link is not found in path
		auto file = fmt::format("lib{}.a", inLink);
		StringList cmd{ exec };
		for (auto& dir : inDirectories)
		{
			cmd.push_back(dir);
		}
		cmd.emplace_back(fmt::format("-print-file-name={}", file));

		auto raw = Commands::subprocessOutput(cmd);
		// auto split = String::split(raw, '\n');

		return !String::equals(file, raw);
	}

	return true;
}
}
