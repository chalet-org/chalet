/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/TargetAdapterVCXProj.hpp"

#include "Builder/ScriptRunner.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
TargetAdapterVCXProj::TargetAdapterVCXProj(const BuildState& inState, const IBuildTarget& inTarget, const std::string& inCwd) :
	m_state(inState),
	m_target(inTarget),
	m_cwd(inCwd)
{
}

/*****************************************************************************/
StringList TargetAdapterVCXProj::getFiles() const
{
	StringList ret;

	if (m_target.isScript())
	{
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto file = fmt::format("{}/{}", m_cwd, script.file());
		if (!Commands::pathExists(file))
			file = script.file();

		ret.emplace_back(std::move(file));
	}

	return ret;
}

/*****************************************************************************/
std::string TargetAdapterVCXProj::getCommand() const
{
	std::string ret;

	auto cwd = Commands::getWorkingDirectory();
	if (!m_cwd.empty())
		Commands::changeWorkingDirectory(m_cwd);

	ScriptType scriptType = ScriptType::None;
	if (m_target.isScript())
	{
		ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto [cmd, type] = scriptRunner.getCommand(script.file(), script.arguments());
		if (!cmd.empty())
		{
			cmd.front() = fmt::format("\"{}\"", cmd.front());
			ret = String::join(cmd);
			scriptType = type;
		}
	}
	else if (m_target.isProcess())
	{
		const auto& process = static_cast<const ProcessBuildTarget&>(m_target);
		StringList cmd;
		cmd.emplace_back(fmt::format("\"{}\"", process.path()));
		for (auto& arg : process.arguments())
		{
			cmd.emplace_back(arg);
		}
		ret = String::join(cmd);

		if (String::contains("python", process.path()))
			scriptType = ScriptType::Python;
	}

	if (!ret.empty())
	{
		if (scriptType == ScriptType::Python)
		{
			ret = fmt::format("cd {}\r\nset PYTHONIOENCODING=utf-8\r\nset PYTHONLEGACYWINDOWSSTDIO=utf-8\r\n{}", m_cwd, ret);
		}
		else
		{
			ret = fmt::format("cd {}\r\n{}", m_cwd, ret);
		}
	}

	if (!cwd.empty())
		Commands::changeWorkingDirectory(cwd);

	return ret;
}
}
