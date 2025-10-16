/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/TargetExportAdapter.hpp"

#include "Builder/CmakeBuilder.hpp"
#include "Builder/MesonBuilder.hpp"
#include "Builder/ScriptRunner.hpp"
#include "Builder/SubChaletBuilder.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildState.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "State/Target/MesonTarget.hpp"
#include "State/Target/ProcessBuildTarget.hpp"
#include "State/Target/ScriptBuildTarget.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/Target/SubChaletTarget.hpp"
#include "State/Target/ValidationBuildTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
constexpr bool kQuotedPaths = true;
}

/*****************************************************************************/
TargetExportAdapter::TargetExportAdapter(const BuildState& inState, const IBuildTarget& inTarget) :
	m_state(inState),
	m_target(inTarget)
{
}

/*****************************************************************************/
bool TargetExportAdapter::generateRequiredFiles(const std::string& inLocation) const
{
	if (m_target.isMeson())
	{
		UNUSED(inLocation);

		const auto& project = static_cast<const MesonTarget&>(m_target);
		MesonBuilder builder(m_state, project, kQuotedPaths);
		if (!builder.createNativeFile()) // TODO: Some hacky stuff in here
			return false;
	}

	return true;
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
	else if (m_target.isSubChalet())
	{
		const auto& subChaletTarget = static_cast<const SubChaletTarget&>(m_target);
		SubChaletBuilder builder(m_state, subChaletTarget, kQuotedPaths);

		auto buildFile = builder.getBuildFile();

		ret.emplace_back(std::move(buildFile));
	}
	else if (m_target.isCMake())
	{
		const auto& project = static_cast<const CMakeTarget&>(m_target);
		CmakeBuilder builder(m_state, project, kQuotedPaths);

		auto buildFile = builder.getBuildFile(true);

		ret.emplace_back(std::move(buildFile));
	}
	else if (m_target.isMeson())
	{
		const auto& project = static_cast<const MesonTarget&>(m_target);
		MesonBuilder builder(m_state, project, kQuotedPaths);

		auto buildFile = builder.getBuildFile(true);

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

	if (m_target.isCMake())
	{
		const auto& project = static_cast<const CMakeTarget&>(m_target);
		CmakeBuilder builder(m_state, project, kQuotedPaths);

		ret.emplace_back(Files::getCanonicalPath(builder.getCacheFile()));
	}
	else if (m_target.isMeson())
	{
		const auto& project = static_cast<const MesonTarget&>(m_target);
		MesonBuilder builder(m_state, project, kQuotedPaths);

		ret.emplace_back(Files::getCanonicalPath(builder.getCacheFile()));
	}

	return ret;
}

/*****************************************************************************/
std::string TargetExportAdapter::getCommand() const
{
	std::string ret;

	auto eol = String::eol();

	// Note: Could be the cwd for script execution, or the project's main cwd
	std::string cwd = m_state.inputs.workingDirectory();

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

		auto& customCwd = process.workingDirectory();
		if (!customCwd.empty())
			cwd = customCwd;

		if (String::contains("python", process.path()))
			scriptType = ScriptType::Python;
	}
	else if (m_target.isCMake())
	{
		const auto& project = static_cast<const CMakeTarget&>(m_target);
		CmakeBuilder builder(m_state, project, kQuotedPaths);

		auto genCmd = builder.getGeneratorCommand();
		// genCmd.front() = fmt::format("\"{}\"", genCmd.front());

		auto buildCmd = builder.getBuildCommand();
		// buildCmd.front() = fmt::format("\"{}\"", buildCmd.front());

		ret = fmt::format("{}{}{}", String::join(genCmd), eol, String::join(buildCmd));

		if (project.install())
		{
			auto installCmd = builder.getInstallCommand();
			ret += fmt::format("{}{}", eol, String::join(installCmd));
		}
	}
	else if (m_target.isMeson())
	{
		const auto& project = static_cast<const MesonTarget&>(m_target);
		MesonBuilder builder(m_state, project, kQuotedPaths);

		auto genCmd = builder.getSetupCommand();
		// genCmd.front() = fmt::format("\"{}\"", genCmd.front());

		auto buildCmd = builder.getBuildCommand();
		// buildCmd.front() = fmt::format("\"{}\"", buildCmd.front());

		ret = fmt::format("{}{}{}", String::join(genCmd), eol, String::join(buildCmd));

		if (project.install())
		{
			auto installCmd = builder.getInstallCommand();
			ret += fmt::format("{}{}", eol, String::join(installCmd));
		}
	}
	else if (m_target.isSubChalet())
	{
		// SubChaletTarget
		const auto& project = static_cast<const SubChaletTarget&>(m_target);
		constexpr bool hasSettings = false;

		SubChaletBuilder builder(m_state, project, kQuotedPaths);

		auto buildCmd = builder.getBuildCommand(hasSettings);
		ret = String::join(buildCmd);

		if (project.install())
		{
			auto installCmd = builder.getInstallCommand(hasSettings);
			ret += fmt::format("{}{}", eol, String::join(installCmd));
		}
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

		std::string searchPaths;
#if defined(CHALET_LINUX) || defined(CHALET_MACOS)
		if (m_target.isProcess())
		{
			auto exprt = "export";
			searchPaths = m_state.workspace.makePathVariable(std::string(), StringList{});
			if (!searchPaths.empty())
			{
				auto key = Environment::getLibraryPathKey();
				auto sep = Environment::getPathSeparator();
				searchPaths = fmt::format("{exprt} {key}=\"{searchPaths}{sep}{key}\" && ",
					FMT_ARG(exprt),
					FMT_ARG(key),
					FMT_ARG(sep),
					FMT_ARG(searchPaths));
			}
		}
		UNUSED(scriptType);
#elif defined(CHALET_WIN32)
		if (scriptType == ScriptType::Python)
		{
			ret = fmt::format("cd {cwd}{eol}set PYTHONIOENCODING=utf-8{eol}set PYTHONLEGACYWINDOWSSTDIO=utf-8{eol}{searchPaths}{ret}",
				FMT_ARG(cwd),
				FMT_ARG(eol),
				FMT_ARG(ret),
				FMT_ARG(searchPaths));
		}
		else
#endif
		{
			ret = fmt::format("cd {cwd}{eol}{searchPaths}{ret}",
				FMT_ARG(cwd),
				FMT_ARG(eol),
				FMT_ARG(ret),
				FMT_ARG(searchPaths));
		}
	}

	return ret;
}

/*****************************************************************************/
std::string TargetExportAdapter::getRunWorkingDirectory() const
{
	if (m_target.isSources())
	{
		const auto& project = static_cast<const SourceTarget&>(m_target);
		if (project.isExecutable())
		{
			auto& cwd = project.runWorkingDirectory();
			if (!cwd.empty())
				return cwd;
		}
	}
	else if (m_target.isCMake())
	{
		const auto& project = static_cast<const CMakeTarget&>(m_target);
		auto& cwd = project.runWorkingDirectory();
		if (!cwd.empty())
			return cwd;
	}
	else if (m_target.isMeson())
	{
		const auto& project = static_cast<const MesonTarget&>(m_target);
		auto& cwd = project.runWorkingDirectory();
		if (!cwd.empty())
			return cwd;
	}

	return m_state.inputs.workingDirectory();
}

/*****************************************************************************/
std::string TargetExportAdapter::getRunWorkingDirectoryWithCurrentWorkingDirectoryAs(const std::string& inAlias) const
{
	const auto& cwd = m_state.inputs.workingDirectory();
	auto path = getRunWorkingDirectory();
	String::replaceAll(path, cwd, inAlias);
	return path;
}
}
