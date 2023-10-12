/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/TargetAdapterPBXProj.hpp"

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
#include "State/Target/ValidationBuildTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
TargetAdapterPBXProj::TargetAdapterPBXProj(const BuildState& inState, const IBuildTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
StringList TargetAdapterPBXProj::getFiles() const
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
	else if (m_target.isValidation())
	{
		const auto& validationTarget = static_cast<const ValidationBuildTarget&>(m_target);
		auto& files = validationTarget.files();
		for (auto& file : files)
		{
			ret.emplace_back(file);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string TargetAdapterPBXProj::getCommand() const
{
	std::string ret;

	ScriptType scriptType = ScriptType::None;
	if (m_target.isScript())
	{
		ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto cmd = scriptRunner.getCommand(script.scriptType(), script.file(), script.arguments());
		if (!cmd.empty())
		{
			cmd.front() = fmt::format("\"{}\"", cmd.front());
			ret = String::join(cmd);
			scriptType = script.scriptType();
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
		bool quotedPaths = false;
		CmakeBuilder builder(m_state, cmakeTarget, quotedPaths);

		auto genCmd = builder.getGeneratorCommand();
		// genCmd.front() = fmt::format("\"{}\"", genCmd.front());

		auto buildCmd = builder.getBuildCommand();
		// buildCmd.front() = fmt::format("\"{}\"", buildCmd.front());

		ret = fmt::format("{}\n{}", String::join(genCmd), String::join(buildCmd));
	}
	else if (m_target.isSubChalet())
	{
		// SubChaletTarget
		const auto& subChaletTarget = static_cast<const SubChaletTarget&>(m_target);
		constexpr bool quotedPaths = true;
		constexpr bool hasSettings = false;
		SubChaletBuilder builder(m_state, subChaletTarget, quotedPaths);
		const auto& targetNames = subChaletTarget.targets();
		for (auto& targetName : targetNames)
		{
			auto buildCmd = builder.getBuildCommand(targetName, hasSettings);
			ret += fmt::format("{}\n", String::join(buildCmd));
		}
	}
	else if (m_target.isValidation())
	{
		const auto& validationTarget = static_cast<const ValidationBuildTarget&>(m_target);
		StringList validateCmd{
			fmt::format("\"{}\"", m_state.tools.chalet()),
			"validate",
			fmt::format("\"{}\"", validationTarget.schema()),
		};
		auto& files = validationTarget.files();
		for (auto& file : files)
		{
			validateCmd.emplace_back(fmt::format("\"{}\"", file));
		}
		ret += fmt::format("{}\n", String::join(validateCmd));
	}

	if (!ret.empty())
	{
		const auto& cwd = m_state.inputs.workingDirectory();
		if (scriptType == ScriptType::Python)
		{
			ret = fmt::format("cd {}\nset PYTHONIOENCODING=utf-8\nset PYTHONLEGACYWINDOWSSTDIO=utf-8\n{}", cwd, ret);
		}
		else
		{
			ret = fmt::format("cd {}\n{}", cwd, ret);
		}
	}

	return ret;
}
}
