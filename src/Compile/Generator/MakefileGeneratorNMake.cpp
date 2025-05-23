/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorNMake.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MakefileGeneratorNMake::MakefileGeneratorNMake(const BuildState& inState) :
	IStrategyGenerator(inState)
{
}

/*****************************************************************************/
void MakefileGeneratorNMake::addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();
	m_hash = inTargetHash;

	m_toolchain->setGenerateDependencies(false);

	const auto& target = inOutputs.target;

	const std::string buildRecipes = getBuildRecipes(inOutputs);

	//
	//
	//
	//
	// ==============================================================================
	std::string makeTemplate = fmt::format(R"makefile(
{buildRecipes}
build_{hash}: {target}
)makefile",
		fmt::arg("hash", m_hash),
		FMT_ARG(buildRecipes),
		FMT_ARG(target));

	m_targetRecipes.emplace_back(std::move(makeTemplate));

	m_toolchain = nullptr;
	m_project = nullptr;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getContents(const std::string& inPath) const
{
	UNUSED(inPath);

	const auto shell = "cmd.exe";

	auto recipes = String::join(m_targetRecipes);

	auto dependency = m_state.environment->getDependencyFile("%");

	std::string depDirs;

	const bool isMsvc = true; // always true currently
	if (!isMsvc)
	{
		depDirs = fmt::format(R"makefile(
{dependency}: ;
.PRECIOUS: {dependency}
)makefile",
			FMT_ARG(dependency));
	}

	//
	//
	//
	//
	// ==============================================================================
	std::string makefileTemplate = String::withByteOrderMark(fmt::format(R"makefile(
.SUFFIXES:

SHELL = {shell}
{recipes}{depDirs}
)makefile",
		// FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(recipes),
		FMT_ARG(depDirs)));

	return makefileTemplate;
}

/*****************************************************************************/
void MakefileGeneratorNMake::reset()
{
	m_targetRecipes.clear();
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEcho(const std::string& inFile) const
{
	// const auto& color = Output::getAnsiStyle(Output::theme().build);
	// const auto& reset = Output::getAnsiStyle(Color::Reset);
	std::string printer;

	if (Output::cleanOutput())
	{
		// auto outFile = m_state.paths.getBuildOutputPath(inFile);
		// auto text = fmt::format("{color}{outFile}{reset}",
		// 	FMT_ARG(color),
		// 	FMT_ARG(outFile),
		// 	FMT_ARG(reset));
		auto outFile = String::getPathFilename(inFile);
		printer = getPrinter(outFile);
	}
	else
	{
		// printer = getPrinter(color);
		printer = getPrinter(std::string());
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getLinkerEcho(const std::string& inFile) const
{
	// const auto& color = Output::getAnsiStyle(Output::theme().build);
	// const auto& reset = Output::getAnsiStyle(Color::Reset);
	std::string printer;

	if (Output::cleanOutput())
	{
		const auto description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

		auto outputFile = m_state.paths.getBuildOutputPath(inFile);
		auto text = fmt::format("{} {}", description, outputFile);
		// auto text = fmt::format("{color}{description} {outputFile}{reset}",
		// 	FMT_ARG(color),
		// 	FMT_ARG(description),
		// 	FMT_ARG(outputFile),
		// 	FMT_ARG(reset));
		printer = getPrinter(text);
	}
	else
	{
		// printer = getPrinter(color);
		printer = getPrinter(std::string());
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getBuildRecipes(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	std::string recipes;

	recipes += getObjBuildRecipes(inOutputs.groups);
	recipes += getTargetRecipe(inOutputs.target, inOutputs.objectListLinker);

	return recipes;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPchBuildRecipe(const StringList& inPches) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool usesPch = m_project->usesPrecompiledHeader();
	if (usesPch)
	{
		const std::string targets = String::join(inPches);
		ret = fmt::format(R"makefile(
pch_{hash}: {targets}
)makefile",
			fmt::arg("hash", m_hash),
			FMT_ARG(targets));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getObjBuildRecipes(const SourceFileGroupList& inGroups)
{
	std::string ret;

	StringList pches;

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
	pches.push_back(pchTarget);

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& object = group->objectFile;
		if (source.empty())
			continue;

		switch (group->type)
		{
			case SourceType::C:
			case SourceType::CPlusPlus:
				ret += getCxxRecipe(source, object, pchTarget, group->type);
				break;

			case SourceType::WindowsResource:
				ret += getRcRecipe(source, object);
				break;

			case SourceType::CxxPrecompiledHeader:
				ret += getPchRecipe(source, object);
				break;

			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus:
			case SourceType::Unknown:
			default:
				break;
		}
	}

	if (!pches.empty())
	{
		ret += getPchBuildRecipe(pches);
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const
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
std::string MakefileGeneratorNMake::getPchRecipe(const std::string& source, const std::string& object)
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
		m_precompiledHeaders.push_back(std::move(pchCache));
		auto pchCompile = String::join(m_toolchain->compilerCxx->getPrecompiledHeaderCommand(source, object, std::string(), std::string()));
		if (!pchCompile.empty())
		{

			std::string compilerEcho;
			if (m_state.environment->type() != ToolchainType::VisualStudio)
			{
				compilerEcho = getCompileEcho(object) + "\n\t";
			}

			ret = fmt::format(R"makefile(
{object}: {source}
	{compilerEcho}{quietFlag}{pchCompile}
)makefile",
				FMT_ARG(object),
				FMT_ARG(source),
				FMT_ARG(compilerEcho),
				FMT_ARG(quietFlag),
				FMT_ARG(pchCompile));
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getRcRecipe(const std::string& source, const std::string& object) const
{
	std::string ret;

	const auto quietFlag = getQuietFlag();

	std::string dependency;
	auto rcCompile = String::join(m_toolchain->compilerWindowsResource->getCommand(source, object, dependency));
	if (!rcCompile.empty())
	{
		const auto compilerEcho = getCompileEcho(source);

		auto nul = Shell::getNull();

		ret = fmt::format(R"makefile(
{object}: {source}
	{compilerEcho}
	{quietFlag}{rcCompile} 1>{nul}
)makefile",
			FMT_ARG(object),
			FMT_ARG(source),
			FMT_ARG(compilerEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(rcCompile),
			FMT_ARG(nul));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCxxRecipe(const std::string& source, const std::string& object, const std::string& pchTarget, const SourceType derivative) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();

	std::string dependency;
	auto cppCompile = String::join(m_toolchain->compilerCxx->getCommand(source, object, dependency, derivative));
	if (!cppCompile.empty())
	{
		std::string compilerEcho;
		if (m_state.environment->type() != ToolchainType::VisualStudio)
		{
			compilerEcho = getCompileEcho(source) + "\n\t";
		}

		ret = fmt::format(R"makefile(
{object}: {source} {pchTarget}
	{compilerEcho}{quietFlag}{cppCompile}
)makefile",
			FMT_ARG(source),
			FMT_ARG(compilerEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(cppCompile),
			FMT_ARG(object),
			FMT_ARG(pchTarget));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getLinkerPreReqs(const StringList& objects) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret = String::join(objects);

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (String::equals(project.name(), m_project->name()))
				break;

			if (List::contains(m_project->projectStaticLinks(), project.name()))
			{
				ret += " " + m_state.paths.getTargetFilename(project);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getQuietFlag() const
{
	return Output::cleanOutput() ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPrinter(const std::string& inPrint) const
{
	if (inPrint == "\\n")
	{
		return "echo.";
	}

	if (inPrint.empty())
	{
		return "prompt"; // This just needs to be a noop
	}

	return fmt::format("echo {}", inPrint);
}
}