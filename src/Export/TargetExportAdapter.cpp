/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/TargetExportAdapter.hpp"

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
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
TargetExportAdapter::TargetExportAdapter(const BuildState& inState, const IBuildTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
StringList TargetExportAdapter::getFiles() const
{
	StringList ret;

	if (m_target.isScript())
	{
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto file = Files::getCanonicalPath(script.file());
		if (!Files::pathExists(file))
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
StringList TargetExportAdapter::getOutputFiles() const
{
	StringList ret;

	if (m_target.isScript())
	{
	}
	else if (m_target.isCMake())
	{
		const auto& cmakeTarget = static_cast<const CMakeTarget&>(m_target);
		bool quotedPaths = true;
		CmakeBuilder builder(m_state, cmakeTarget, quotedPaths);

		ret.emplace_back(Files::getCanonicalPath(builder.getCacheFile()));
	}
	else if (m_target.isSubChalet())
	{
	}
	else if (m_target.isValidation())
	{
	}

	return ret;
}

/*****************************************************************************/
std::string TargetExportAdapter::getCommand() const
{
	std::string ret;

	auto eol = String::eol();

	const auto& cwd = m_state.inputs.workingDirectory();
	ScriptType scriptType = ScriptType::None;
	if (m_target.isScript())
	{
		ScriptRunner scriptRunner(m_state.inputs, m_state.tools);
		const auto& script = static_cast<const ScriptBuildTarget&>(m_target);

		auto cmd = scriptRunner.getCommand(script.scriptType(), script.file(), script.arguments(), true);
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
		bool quotedPaths = true;
		CmakeBuilder builder(m_state, cmakeTarget, quotedPaths);

		auto genCmd = builder.getGeneratorCommand();
		// genCmd.front() = fmt::format("\"{}\"", genCmd.front());

		auto buildCmd = builder.getBuildCommand();
		// buildCmd.front() = fmt::format("\"{}\"", buildCmd.front());

		ret = fmt::format("{}{}{}", String::join(genCmd), eol, String::join(buildCmd));

		if (cmakeTarget.install())
		{
			auto installCmd = builder.getInstallCommand();
			ret += fmt::format("{}{}", eol, String::join(installCmd));
		}
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
	else if (m_target.isValidation())
	{
		const auto& validationTarget = static_cast<const ValidationBuildTarget&>(m_target);
		StringList validateCmd{
			fmt::format("\"{}\"", m_state.inputs.appPath()),
			"validate",
			fmt::format("\"{}\"", validationTarget.schema()),
		};
		auto& files = validationTarget.files();
		for (auto& file : files)
		{
			validateCmd.emplace_back(fmt::format("\"{}\"", file));
		}
		ret = String::join(validateCmd);
	}

	if (!ret.empty())
	{
		ret += eol;

		if (scriptType == ScriptType::Python)
		{
			ret = fmt::format("cd {cwd}{eol}set PYTHONIOENCODING=utf-8{eol}set PYTHONLEGACYWINDOWSSTDIO=utf-8{eol}{ret}", FMT_ARG(cwd), FMT_ARG(eol), FMT_ARG(ret));
		}
		else
		{
			ret = fmt::format("cd {}{}{}", cwd, eol, ret);
		}
	}

	return ret;
}
}
