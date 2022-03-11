/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocksProjectExporter.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksProjectExporter::CodeBlocksProjectExporter(const BuildState& inState) :
	IProjectExporter(inState, ExportKind::CodeBlocks)
{
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::validate()
{
	if (!m_state.environment->isGcc())
	{
		Diagnostic::fatalError("CodeBlocks project exporter requires a GCC toolchain (set with --toolchain/-t).");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::generate()
{
	auto exportDirectory = "chalet_export";
	if (!Commands::pathExists(exportDirectory))
	{
		Commands::makeDirectory(exportDirectory);
	}

	m_cwd = m_state.inputs.workingDirectory();
	Commands::changeWorkingDirectory(fmt::format("{}/{}", m_cwd, exportDirectory));

	bool hasProjects = false;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto relativeFile = fmt::format("{}.cbp", target->name());
			auto contents = getProjectContent(static_cast<const SourceTarget&>(*target));

			if (!Commands::createFileWithContents(relativeFile, contents))
			{
				Diagnostic::fatalError("There was a problem creating the CodeBlocks project file for the target: {}", target->name());
				return false;
			}

			hasProjects = true;
		}
	}

	if (hasProjects)
	{
		auto workspaceFile = "project.workspace";
		auto contents = getWorkspaceContent();

		if (!Commands::createFileWithContents(workspaceFile, contents))
		{
			Diagnostic::fatalError("There was a problem creating the CodeBlocks workspace file.");
			return false;
		}
	}

	Commands::changeWorkingDirectory(m_cwd);

	return true;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectContent(const SourceTarget& inTarget) const
{
	std::string ret;

	m_state.paths.setBuildDirectoriesBasedOnProjectKind(inTarget);

	const auto& objDir = m_state.paths.objDir();

	const auto& name = inTarget.name();
	auto compilerOptions = getProjectCompileOptions(inTarget);
	auto compilerIncludes = getProjectCompileIncludes(inTarget);
	auto linkerOptions = getProjectLinkerOptions(inTarget);
	auto linkerLibDirs = getProjectLinkerLibDirs(inTarget);
	auto linkerLinks = getProjectLinkerLinks(inTarget);

	auto units = getProjectUnits(inTarget);

	auto filename = m_state.paths.getTargetFilename(inTarget);

	inTarget.files();

	std::string sharedProperties;
	char outputType = '1';
	if (inTarget.kind() == SourceKind::SharedLibrary)
	{
		outputType = '3';
		sharedProperties += fmt::format(R"xml(
				<Option createDefFile="1" />
				<Option createStaticLib="1" />)xml");
	}
	else if (inTarget.kind() == SourceKind::StaticLibrary)
	{
		outputType = '2';
	}
#if defined(CHALET_WIN32)
	else if (inTarget.windowsSubSystem() == WindowsSubSystem::Windows)
	{
		outputType = '0';
	}
#endif

	ret = fmt::format(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<CodeBlocks_project_file>
	<FileVersion major="1" minor="6" />
	<Project>
		<Option title="{name}" />
		<Option pch_mode="1" />
		<Option compiler="gcc" />
		<Build>
			<!--<Script file="" />-->
			<Target title="Release">
				<Option output="{cwd}/{filename}" />
				<Option working_dir="{cwd}" />
				<Option object_output="{cwd}/{objDir}" />
				<Option type="{outputType}" />
				<Option compiler="gcc" />
				<Option use_console_runner="1" />
				<!--<Option external_deps="" />--><!-- projectStaticLinks - resolved -->
				<!--<Option additional_output="" />-->{sharedProperties}
				<Compiler>{compilerOptions}{compilerIncludes}
				</Compiler>
				<Linker>{linkerOptions}{linkerLibDirs}{linkerLinks}
				</Linker>
				<!--<ExtraCommands>
					<Add before=""/>
					<Add after=""/>
					<Mode after="always" />
				</ExtraCommands>-->
			</Target>
		</Build>{units}
		<Extensions>
			<code_completion />
			<envvars />
			<debugger />
			<lib_finder disable_auto="1" />
		</Extensions>
	</Project>
</CodeBlocks_project_file>
)xml",
		FMT_ARG(name),
		FMT_ARG(filename),
		fmt::arg("cwd", m_cwd),
		FMT_ARG(objDir),
		FMT_ARG(outputType),
		FMT_ARG(sharedProperties),
		FMT_ARG(compilerOptions),
		FMT_ARG(compilerIncludes),
		FMT_ARG(linkerOptions),
		FMT_ARG(linkerLibDirs),
		FMT_ARG(linkerLinks),
		FMT_ARG(units));

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectCompileOptions(const SourceTarget& inTarget) const
{
	UNUSED(inTarget);
	std::string ret;

	ret += fmt::format(R"xml(
					<Add option="-O2" />)xml");

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectCompileIncludes(const SourceTarget& inTarget) const
{
	std::string ret;

	for (auto& dir : inTarget.includeDirs())
	{
		std::string resolved;
		if (!Commands::pathExists(dir))
			resolved = fmt::format("{}/{}", m_cwd, dir);
		else
			resolved = dir;

		ret += fmt::format(R"xml(
					<Add directory="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectLinkerOptions(const SourceTarget& inTarget) const
{
	UNUSED(inTarget);
	std::string ret;

	ret = fmt::format(R"xml(
					<Add option="-s" />)xml");

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectLinkerLibDirs(const SourceTarget& inTarget) const
{
	std::string ret;

	for (auto& dir : inTarget.libDirs())
	{
		std::string resolved;
		if (!Commands::pathExists(dir))
			resolved = fmt::format("{}/{}", m_cwd, dir);
		else
			resolved = dir;

		ret += fmt::format(R"xml(
					<Add directory="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	const auto& buildDir = m_state.paths.buildOutputDir();
	ret += fmt::format(R"xml(
					<Add directory="{cwd}/{buildDir}" />)xml",
		fmt::arg("cwd", m_cwd),
		FMT_ARG(buildDir));

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectLinkerLinks(const SourceTarget& inTarget) const
{
	std::string ret;

	for (auto& link : inTarget.links())
	{
		ret += fmt::format(R"xml(
					<Add library="{link}" />)xml",
			FMT_ARG(link));
	}

	for (auto& link : inTarget.staticLinks())
	{
		ret += fmt::format(R"xml(
					<Add library="{link}" />)xml",
			FMT_ARG(link));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectUnits(const SourceTarget& inTarget) const
{
	std::string ret;

	for (auto& file : inTarget.files())
	{
		std::string resolved;
		if (!Commands::pathExists(file))
			resolved = fmt::format("{}/{}", m_cwd, file);
		else
			resolved = file;

		ret += fmt::format(R"xml(
		<Unit filename="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getWorkspaceContent() const
{
	std::string ret;

	int i = 0;
	std::string projects;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			projects += getWorkspaceProject(project, i == 0 && project.isExecutable());
			++i;
		}
	}

	const auto& workspaceName = m_state.workspace.workspaceName();
	ret = fmt::format(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<CodeBlocks_workspace_file>
	<Workspace title="{workspaceName}">{projects}
	</Workspace>
</CodeBlocks_workspace_file>
)xml",
		FMT_ARG(workspaceName),
		FMT_ARG(projects));

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getWorkspaceProject(const SourceTarget& inTarget, const bool inActive) const
{
	std::string ret;

	auto& target = inTarget.name();
	auto active = inActive ? " active=\"1\"" : "";
	auto& projectStaticLinks = inTarget.projectStaticLinks();
	if (!projectStaticLinks.empty())
	{
		for (auto& link : projectStaticLinks)
		{
			ret += fmt::format(R"xml(
		<Project filename="{target}.cbp"{active}>
			<Depends filename="{link}.cbp" />
		</Project>)xml",
				FMT_ARG(target),
				FMT_ARG(active),
				FMT_ARG(link));
		}
	}
	else
	{
		return fmt::format(R"xml(
		<Project filename="{target}.cbp"{active} />)xml",
			FMT_ARG(target),
			FMT_ARG(active));
	}

	return ret;
}
}
