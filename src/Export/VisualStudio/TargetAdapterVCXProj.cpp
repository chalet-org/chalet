/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/TargetAdapterVCXProj.hpp"

#include "Builder/CmakeBuilder.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
TargetAdapterVCXProj::TargetAdapterVCXProj(const BuildState& inState, const IBuildTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
StringList TargetAdapterVCXProj::getFiles() const
{
	StringList ret;

	const auto& cwd = m_state.inputs.workingDirectory();

	if (m_target.isScript())
	{
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto file = fmt::format("{}/{}", cwd, script.file());
		if (!Commands::pathExists(file))
			file = script.file();

		ret.emplace_back(std::move(file));
	}
	else if (m_target.isCMake())
	{
		const auto& cmakeTarget = static_cast<const CMakeTarget&>(m_target);
		bool quotedPaths = true;
		CmakeBuilder builder(m_state, cmakeTarget, quotedPaths);

		auto buildFile = builder.getBuildFile(true);

		ret.emplace_back(std::move(buildFile));
	}
	else if (m_target.isSubChalet())
	{
		const auto& subChaletTarget = static_cast<const SubChaletTarget&>(m_target);
		bool quotedPaths = true;
		SubChaletBuilder builder(m_state, subChaletTarget, quotedPaths);

		auto buildFile = builder.getBuildFile();

		ret.emplace_back(std::move(buildFile));
	}

	return ret;
}

/*****************************************************************************/
std::string TargetAdapterVCXProj::getCommand() const
{
	std::string ret;

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
	else if (m_target.isCMake())
	{
		const auto& cmakeTarget = static_cast<const CMakeTarget&>(m_target);
		bool quotedPaths = true;
		CmakeBuilder builder(m_state, cmakeTarget, quotedPaths);

		auto genCmd = builder.getGeneratorCommand();
		// genCmd.front() = fmt::format("\"{}\"", genCmd.front());

		auto buildCmd = builder.getBuildCommand();
		// buildCmd.front() = fmt::format("\"{}\"", buildCmd.front());

		ret = fmt::format("{}\r\n{}", String::join(genCmd), String::join(buildCmd));
	}
	else if (m_target.isSubChalet())
	{
		// SubChaletTarget
		const auto& subChaletTarget = static_cast<const SubChaletTarget&>(m_target);
		constexpr bool quotedPaths = true;
		constexpr bool hasSettings = false;
		SubChaletBuilder builder(m_state, subChaletTarget, quotedPaths);
		auto buildCmd = builder.getBuildCommand(hasSettings);

		ret = String::join(buildCmd);
	}

	if (!ret.empty())
	{
		const auto& cwd = m_state.inputs.workingDirectory();
		if (scriptType == ScriptType::Python)
		{
			ret = fmt::format("cd {}\r\nset PYTHONIOENCODING=utf-8\r\nset PYTHONLEGACYWINDOWSSTDIO=utf-8\r\n{}", cwd, ret);
		}
		else
		{
			ret = fmt::format("cd {}\r\n{}", cwd, ret);
		}
	}

	return ret;
}
}
