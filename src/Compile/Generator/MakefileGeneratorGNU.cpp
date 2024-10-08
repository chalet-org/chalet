/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorGNU.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
// #include "Terminal/Shell.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MakefileGeneratorGNU::MakefileGeneratorGNU(const BuildState& inState) :
	IStrategyGenerator(inState)
{
}

/*****************************************************************************/
void MakefileGeneratorGNU::addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();
	m_hash = inTargetHash;

	const auto& target = inOutputs.target;

	m_fileCount = ((inOutputs.groups.size()) / 10) + 1;
	const std::string buildRecipes = getBuildRecipes(inOutputs);
	const auto printer = getPrinter();

	std::string depends;
	for (auto& group : inOutputs.groups)
	{
		if (group->dependencyFile.empty())
			continue;

		depends += ' ';
		depends += group->dependencyFile;
	}

	auto dependency = m_state.environment->getDependencyFile("%");

	//
	//
	//
	//
	// ==============================================================================
	std::string makeTemplate = fmt::format(R"makefile(
{buildRecipes}
build_{hash}: {target}
	@{printer}
.PHONY: build_{hash}

.PRECIOUS: {dependency}
{dependency}: ;

-include{depends}
)makefile",
		fmt::arg("hash", m_hash),
		FMT_ARG(printer),
		FMT_ARG(buildRecipes),
		FMT_ARG(target),
		FMT_ARG(dependency),
		FMT_ARG(depends));

	m_targetRecipes.emplace_back(std::move(makeTemplate));

	m_toolchain = nullptr;
	m_project = nullptr;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getContents(const std::string& inPath) const
{
	UNUSED(inPath);

	// const auto suffixes = String::getPrefixed(m_fileExtensions, ".");

#if defined(CHALET_WIN32)
	const auto shell = "cmd.exe";
#else
	const auto shell = "/bin/sh";
#endif

	auto recipes = String::join(m_targetRecipes);

	std::string makeTemplate = fmt::format(R"makefile(# Generated by Chalet

.SUFFIXES:

SHELL := {shell}{recipes}
)makefile",
		// FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(recipes));

	return makeTemplate;
}

/*****************************************************************************/
void MakefileGeneratorGNU::reset()
{
	m_targetRecipes.clear();
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getBuildRecipes(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	std::string recipes;

	recipes += getObjBuildRecipes(inOutputs.groups);
	recipes += getTargetRecipe(inOutputs.target, inOutputs.objectListLinker);

	return recipes;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getObjBuildRecipes(const SourceFileGroupList& inGroups)
{
	std::string ret;

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& object = group->objectFile;
		const auto& dependency = group->dependencyFile;
		if (source.empty())
			continue;

		switch (group->type)
		{
			case SourceType::C:
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus:
				ret += getCxxRecipe(source, object, dependency, pchTarget, group->type);
				break;

			case SourceType::WindowsResource:
				ret += getRcRecipe(source, object, dependency);
				break;

			case SourceType::CxxPrecompiledHeader:
				ret += getPchRecipe(source, object, dependency);
				break;

			case SourceType::Unknown:
			default:
				break;
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEcho(const std::string& inFile) const
{
	const auto& color = Output::getAnsiStyle(Output::theme().build);
	const auto& reset = Output::getAnsiStyle(Color::Reset);
	std::string printer;

	if (Output::cleanOutput())
	{
		auto outputFile = m_state.paths.getBuildOutputPath(inFile);
		auto text = fmt::format("   {color}{outputFile}{reset}",
			FMT_ARG(color),
			FMT_ARG(outputFile),
			FMT_ARG(reset));
		printer = getPrinter(text, true);
	}
	else
	{
		printer = getPrinter(std::string(color));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getLinkerEcho(const std::string& inFile) const
{
	const auto& color = Output::getAnsiStyle(Output::theme().build);
	const auto& reset = Output::getAnsiStyle(Color::Reset);
	std::string printer;

	if (Output::cleanOutput())
	{
		const auto description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

		auto outputFile = m_state.paths.getBuildOutputPath(inFile);
		const auto text = fmt::format("   {color}{description} {outputFile}{reset}",
			FMT_ARG(color),
			FMT_ARG(description),
			FMT_ARG(outputFile),
			FMT_ARG(reset));
		printer = getPrinter(text, true);
	}
	else
	{
		printer = getPrinter(std::string(color));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getPchRecipe(const std::string& source, const std::string& object, const std::string& dependency)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const bool usePch = m_project->usesPrecompiledHeader();

	const auto& objDir = m_state.paths.objDir();
	auto pchCache = fmt::format("{}/{}", objDir, source);

	if (usePch && !List::contains(m_precompiledHeaders, pchCache))
	{
		const auto quietFlag = getQuietFlag();
		m_precompiledHeaders.emplace_back(std::move(pchCache));

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(object);
			auto filename = String::getPathFilename(object);

			std::string lastArch;
			for (auto& arch : m_state.inputs.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);
				auto dependencies = source;

				if (!lastArch.empty())
				{
					dependencies += fmt::format(" {}_{}/{}", baseFolder, lastArch, filename);
				}

				auto pchCompile = String::join(m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, outObject, dependency, arch));
				if (!pchCompile.empty())
				{
					auto pch = String::getPathFolderBaseName(object);
					String::replaceAll(pch, fmt::format("{}/", objDir), "");
					pch += fmt::format(" ({})", arch);
					const auto compileEcho = getCompileEcho(pch);

					ret += fmt::format(R"makefile(
{outObject}: {dependencies} | {dependency}
	{compileEcho}
	{quietFlag}{pchCompile}
)makefile",
						FMT_ARG(outObject),
						FMT_ARG(dependencies),
						FMT_ARG(compileEcho),
						FMT_ARG(quietFlag),
						FMT_ARG(pchCompile),
						FMT_ARG(dependency));
				}
				lastArch = arch;
			}
		}
		else
#endif
		{
			auto pchCompile = String::join(m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, object, dependency, std::string()));
			if (!pchCompile.empty())
			{
				auto pch = String::getPathFolderBaseName(object);
				String::replaceAll(pch, fmt::format("{}/", objDir), "");
				const auto compileEcho = getCompileEcho(pch);

				ret += fmt::format(R"makefile(
{object}: {source} | {dependency}
	{compileEcho}
	{quietFlag}{pchCompile}
)makefile",
					FMT_ARG(object),
					FMT_ARG(source),
					FMT_ARG(compileEcho),
					FMT_ARG(quietFlag),
					FMT_ARG(pchCompile),
					FMT_ARG(dependency));
			}
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getRcRecipe(const std::string& source, const std::string& object, const std::string& dependency) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto compileEcho = getCompileEcho(source);

	auto rcCompile = String::join(m_toolchain->compilerWindowsResource->getCommand(source, object, dependency));
	if (!rcCompile.empty())
	{
		std::string makeDependency;
		if (m_toolchain->compilerWindowsResource->generateDependencies() && m_state.toolchain.isCompilerWindowsResourceLLVMRC())
		{
			makeDependency = fmt::format("\n\t@{}", getFallbackMakeDependsCommand(dependency, object, source));
		}

		ret += fmt::format(R"makefile(
{object}: {source} | {dependency}
	{compileEcho}
	{quietFlag}{rcCompile}{makeDependency}
)makefile",
			FMT_ARG(object),
			FMT_ARG(source),
			FMT_ARG(dependency),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(rcCompile),
			FMT_ARG(makeDependency));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCxxRecipe(const std::string& source, const std::string& object, const std::string& dependency, const std::string& pchTarget, const SourceType derivative) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto compileEcho = getCompileEcho(source);

	auto cppCompile = String::join(m_toolchain->compilerCxx->getCommand(source, object, dependency, derivative));
	if (!cppCompile.empty())
	{
		std::string pch = pchTarget;

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS && m_project->usesPrecompiledHeader())
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);
			auto& lastArch = m_state.inputs.universalArches().back();
			pch = fmt::format("{}_{}/{}", baseFolder, lastArch, filename);
		}
		else
#endif
		{
			pch = pchTarget;
		}

		ret += fmt::format(R"makefile(
{object}: {source} {pch} | {dependency}
	{compileEcho}
	{quietFlag}{cppCompile}
)makefile",
			FMT_ARG(source),
			FMT_ARG(object),
			FMT_ARG(dependency),
			FMT_ARG(pch),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(cppCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();

	const auto preReqs = getLinkerPreReqs(objects);

	const auto linkerCommand = String::join(m_toolchain->getOutputTargetCommand(linkerTarget, objects));
	if (!linkerCommand.empty())
	{
		const auto linkerEcho = getLinkerEcho(linkerTarget);

		ret = fmt::format(R"makefile(
{linkerTarget}: {preReqs}
	{linkerEcho}
	{quietFlag}{linkerCommand}
)makefile",
			FMT_ARG(linkerTarget),
			FMT_ARG(preReqs),
			FMT_ARG(linkerEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(linkerCommand));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getLinkerPreReqs(const StringList& objects) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret = String::join(objects);

	u32 count = 0;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (String::equals(project.name(), m_project->name()))
				break;

			/*if (List::contains(m_project->links(), project.name()))
			{
				if (count == 0)
					ret += " |";
				ret += " " + m_state.paths.getTargetFilename(project);
				++count;
			}
			else
			*/
			if (List::contains(m_project->projectStaticLinks(), project.name()))
			{
				if (count == 0)
					ret += " |";
				ret += " " + m_state.paths.getTargetFilename(project);
				++count;
			}
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getQuietFlag() const
{
	return Output::cleanOutput() ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getFallbackMakeDependsCommand(const std::string& inDependencyFile, const std::string& object, const std::string& source) const
{
	std::string contents = fmt::format("{}: \\\\\\n  {}\\n", object, source);
	return fmt::format("echo \"{contents}\" > \"{inDependencyFile}\"",
		FMT_ARG(contents),
		FMT_ARG(inDependencyFile));
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getPrinter(const std::string& inPrint, const bool inNewLine) const
{
#if defined(CHALET_WIN32)
	if (inPrint == "\\n")
	{
		return "echo.";
	}
	else if (inPrint.empty())
	{
		return "rem";
	}

	if (inNewLine)
		return fmt::format("echo {}", inPrint);
	else
		return fmt::format("echo|set /p CMD_NOLINE=\"{}\"", inPrint);
#else
	if (inPrint.empty())
	{
		return ":";
	}

	return fmt::format("printf '{}{}'", inPrint, inNewLine ? "\\n" : "");
#endif
}
}
