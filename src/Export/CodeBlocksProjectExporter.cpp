/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/CodeBlocksProjectExporter.hpp"

#include "Compile/CompileToolchainController.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Export/CodeBlocks/CodeBlocksWorkspaceGen.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CodeBlocksProjectExporter::CodeBlocksProjectExporter(const CommandLineInputs& inInputs) :
	IProjectExporter(inInputs, ExportKind::CodeBlocks)
{
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getMainProjectOutput(const BuildState& inState)
{
	if (m_directory.empty())
	{
		if (!useProjectBuildDirectory(".codeblocks"))
			return std::string();
	}

	auto project = getProjectName(inState);
	return fmt::format("{}/{}.workspace", m_directory, project);
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getMainProjectOutput()
{
	chalet_assert(!m_states.empty(), "states were empty getting project name");
	return getMainProjectOutput(*m_states.front());
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectTypeName() const
{
	return std::string("CodeBlocks");
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::validate(const BuildState& inState)
{
	if (!inState.environment->isGcc())
	{
		Diagnostic::error("CodeBlocks project format requires a GCC toolchain (set with --toolchain/-t).");
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CodeBlocksProjectExporter::generateProjectFiles()
{
	auto workspaceFile = getMainProjectOutput();
	if (workspaceFile.empty())
		return false;

	auto state = getAnyBuildStateButPreferDebug();
	chalet_assert(state != nullptr, "");
	if (state != nullptr)
	{
		bool hasSourceTargets = false;
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				auto relativeFile = fmt::format("{}/cbp/{}.cbp", m_directory, target->name());
				auto contents = getProjectContent(target->name());
				if (!Commands::createFileWithContents(relativeFile, contents))
				{
					Diagnostic::error("There was a problem creating the CodeBlocks project file for the target: {}", target->name());
					return false;
				}

				hasSourceTargets = true;
			}
		}

		if (hasSourceTargets)
		{
			auto allBuildTargetName = getAllBuildTargetName();
			CodeBlocksWorkspaceGen workspaceGen(m_states, m_debugConfiguration, allBuildTargetName);
			if (!workspaceGen.saveToFile(workspaceFile))
			{
				Diagnostic::error("There was a problem creating the CodeBlocks workspace file.");
				return false;
			}
		}
	}

	return true;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectContent(const std::string& inName) const
{
	std::string ret;

	std::string buildConfigurations;

	std::string compiler;
	const SourceTarget* thisTarget = nullptr;
	for (auto& state : m_states)
	{
		for (auto& target : state->targets)
		{
			if (target->isSources())
			{
				if (!String::equals(inName, target->name()))
					continue;

				if (thisTarget == nullptr)
					thisTarget = static_cast<const SourceTarget*>(target.get());

				const auto& sourceTarget = static_cast<const SourceTarget&>(*target);
				state->paths.setBuildDirectoriesBasedOnProjectKind(sourceTarget);

				auto toolchain = std::make_unique<CompileToolchainController>(sourceTarget);
				if (!toolchain->initialize(*state))
				{
					Diagnostic::error("Error preparing the toolchain for project: {}", sourceTarget.name());
					continue;
				}

				if (compiler.empty())
				{
					if (state->environment->isClang())
						compiler = "clang";
					else
						compiler = "gcc";
				}

				buildConfigurations += getProjectBuildConfiguration(*state, sourceTarget, *toolchain);
			}
		}
	}

	chalet_assert(thisTarget != nullptr, "");
	auto units = getProjectUnits(*thisTarget);

	ret = fmt::format(R"xml(<?xml version="1.0" encoding="UTF-8" standalone="yes" ?>
<CodeBlocks_project_file>
	<FileVersion major="1" minor="6" />
	<Project>
		<Option title="{name}" />
		<Option pch_mode="1" />
		<Option compiler="{compiler}" />
		<Build>
			<!--<Script file="" />-->{buildConfigurations}
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
		fmt::arg("name", inName),
		FMT_ARG(compiler),
		FMT_ARG(buildConfigurations),
		FMT_ARG(units));

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectBuildConfiguration(const BuildState& inState, const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const
{
	std::string ret;

	const auto& configName = inState.configuration.name();
	const auto& objDir = inState.paths.objDir();

	auto filename = inState.paths.getTargetFilename(inTarget);
	auto compilerOptions = getProjectCompileOptions(inTarget, inToolchain);
	auto compilerIncludes = getProjectCompileIncludes(inTarget);

	std::string linkerOptions;
	std::string linkerLibDirs;
	std::string linkerLinks;

	if (!inTarget.isStaticLibrary())
	{
		linkerOptions = getProjectLinkerOptions(inToolchain);
		linkerLibDirs = getProjectLinkerLibDirs(inState, inTarget);
		linkerLinks = getProjectLinkerLinks(inTarget);
	}

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

	const auto& cwd = workingDirectory();

	ret = fmt::format(R"xml(
			<Target title="{configName}">
				<Option output="{cwd}/{filename}" />
				<Option working_dir="{cwd}" />
				<Option object_output="{cwd}/{objDir}" />
				<Option type="{outputType}" />
				<!--<Option compiler="gcc" />-->
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
			</Target>)xml",
		FMT_ARG(configName),
		FMT_ARG(filename),
		FMT_ARG(cwd),
		FMT_ARG(objDir),
		FMT_ARG(outputType),
		FMT_ARG(sharedProperties),
		FMT_ARG(compilerOptions),
		FMT_ARG(compilerIncludes),
		FMT_ARG(linkerOptions),
		FMT_ARG(linkerLibDirs),
		FMT_ARG(linkerLinks));

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectCompileOptions(const SourceTarget& inTarget, const CompileToolchainController& inToolchain) const
{
	std::string ret;

	auto derivative = inTarget.getDefaultSourceType();

	StringList argList;
	inToolchain.compilerCxx->getCommandOptions(argList, derivative);
	if (inTarget.usesPrecompiledHeader())
	{
		const auto& cwd = workingDirectory();
		auto pch = inTarget.precompiledHeader();
		auto resolved = fmt::format("{}/{}", cwd, pch);
		if (!Commands::pathExists(pch))
			resolved = pch;

		/*for (auto& dir : inTarget.includeDirs())
		{
			if (String::startsWith(dir, pch))
			{
				String::replaceAll(pch, fmt::format("{}/", dir), "");
				break;
			}
		}*/
		argList.emplace_back(fmt::format("-include &quot;{}&quot;", resolved));
	}

	for (auto& arg : argList)
	{
		ret += fmt::format(R"xml(
					<Add option="{arg}" />)xml",
			FMT_ARG(arg));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectCompileIncludes(const SourceTarget& inTarget) const
{
	std::string ret;

	const auto& cwd = workingDirectory();
	for (auto& dir : inTarget.includeDirs())
	{
		auto resolved = fmt::format("{}/{}", cwd, dir);
		if (!Commands::pathExists(dir))
			resolved = dir;

		ret += fmt::format(R"xml(
					<Add directory="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectLinkerOptions(const CompileToolchainController& inToolchain) const
{
	std::string ret;

	StringList argList;
	inToolchain.linker->getCommandOptions(argList);

	for (auto& arg : argList)
	{
		ret = fmt::format(R"xml(
					<Add option="{arg}" />)xml",
			FMT_ARG(arg));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectLinkerLibDirs(const BuildState& inState, const SourceTarget& inTarget) const
{
	std::string ret;

	const auto& cwd = workingDirectory();
	for (auto& dir : inTarget.libDirs())
	{
		auto resolved = fmt::format("{}/{}", cwd, dir);
		if (!Commands::pathExists(dir))
			resolved = dir;

		ret += fmt::format(R"xml(
					<Add directory="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	const auto& buildDir = inState.paths.buildOutputDir();
	ret += fmt::format(R"xml(
					<Add directory="{cwd}/{buildDir}" />)xml",
		FMT_ARG(cwd),
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

	const auto& cwd = workingDirectory();

	/*if (inTarget.usesPrecompiledHeader())
	{
		const auto& pch = inTarget.precompiledHeader();
		auto resolved = fmt::format("{}/{}", cwd, pch);
		if (!Commands::pathExists(pch))
			resolved = pch;

		ret += fmt::format(R"xml(
		<Unit filename="{resolved}">
			<Option compile="1" />
			<Option weight="0" />
		</Unit>)xml",
			FMT_ARG(resolved));
	}*/

	for (auto& file : inTarget.files())
	{
		auto resolved = fmt::format("{}/{}", cwd, file);
		if (!Commands::pathExists(file))
			resolved = file;

		ret += fmt::format(R"xml(
		<Unit filename="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	auto headers = inTarget.getHeaderFiles();
	for (auto& file : headers)
	{
		auto resolved = fmt::format("{}/{}", cwd, file);
		if (!Commands::pathExists(file))
			resolved = file;

		ret += fmt::format(R"xml(
		<Unit filename="{resolved}" />)xml",
			FMT_ARG(resolved));
	}

	return ret;
}

/*****************************************************************************/
std::string CodeBlocksProjectExporter::getProjectName(const BuildState& inState) const
{
	const auto& workspaceName = inState.workspace.metadata().name();
	return !workspaceName.empty() ? workspaceName : std::string("project");
}

}
