/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorNMake.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MakefileGeneratorNMake::MakefileGeneratorNMake(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	// m_generateDependencies = !Environment::isContinuousIntegrationServer();
	m_generateDependencies = false;
}

/*****************************************************************************/
void MakefileGeneratorNMake::addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();
	m_hash = inTargetHash;

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

	const auto& depDir = m_state.paths.depDir();
	// const auto suffixes = String::getPrefixed(m_fileExtensions, ".");
	const auto shell = "cmd.exe";

	auto recipes = String::join(m_targetRecipes);

	std::string depDirs;

	const bool isMsvc = true; // always true currently
	if (!isMsvc)
	{
		depDirs = fmt::format(R"makefile(
{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d
)makefile",
			FMT_ARG(depDir));
	}

	//
	//
	//
	//
	// ==============================================================================
	std::string makefileTemplate = fmt::format(R"makefile(
.SUFFIXES:

SHELL = {shell}
{recipes}{depDirs}
)makefile",
		// FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(recipes),
		FMT_ARG(depDirs));

	// if (!isBash)
	// {
	// 	String::replaceAll(makefileTemplate, "/%)", "\\\\\\\\%)");
	// 	String::replaceAll(makefileTemplate, "/", "\\\\");
	// }

	return makefileTemplate;
}

/*****************************************************************************/
void MakefileGeneratorNMake::reset()
{
	m_targetRecipes.clear();
	// m_fileExtensions.clear();
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoSources(const std::string& file) const
{
	const auto blue = getBuildColor();
	std::string printer;

	if (Output::cleanOutput())
	{
		auto outFile = String::getPathFilename(file);
		printer = getPrinter(fmt::format("{blue}{outFile}", FMT_ARG(blue), FMT_ARG(outFile)));
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoLinker(const std::string& file) const
{
	const auto blue = getBuildColor();
	std::string printer;

	if (Output::cleanOutput())
	{
		// Note: If trying to echo "=   Linking (file)", nmake will eat up the additional spaces,
		//   so both them and the symbol are taken out since

		// const auto arrow = Unicode::rightwardsTripleArrow();

		const std::string description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

		printer = getPrinter(fmt::format("{blue}{description} {file}", FMT_ARG(blue), FMT_ARG(description), FMT_ARG(file)));
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getBuildRecipes(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	/*for (auto& ext : inOutputs.fileExtensions)
	{
		List::addIfDoesNotExist(m_fileExtensions, ext);
	}*/

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

	const bool usesPch = m_project->usesPch();
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

	const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());
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
				ret += getCppRecipe(source, object, pchTarget);
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

	const auto linkerTargetBase = m_state.paths.getTargetBasename(*m_project);
	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand(linkerTarget, objects, linkerTargetBase));

	if (!linkerCommand.empty())
	{
		const auto compileEcho = getCompileEchoLinker(linkerTarget);
		const auto printer = getPrinter("\\n");

		ret = fmt::format(R"makefile(
{linkerTarget}: {preReqs}
	{compileEcho}
	{quietFlag}{linkerCommand}
	@{printer}
)makefile",
			FMT_ARG(linkerTarget),
			FMT_ARG(preReqs),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(linkerCommand),
			FMT_ARG(printer));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPchRecipe(const std::string& source, const std::string& object)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const bool usePch = m_project->usesPch();

	const auto& objDir = m_state.paths.objDir();
	auto pchCache = fmt::format("{}/{}", objDir, source);

	if (usePch && !List::contains(m_precompiledHeaders, pchCache))
	{
		const auto quietFlag = getQuietFlag();
		m_precompiledHeaders.push_back(std::move(pchCache));
		// const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());

		auto pchCompile = String::join(m_toolchain->getPchCompileCommand(source, object, m_generateDependencies, std::string(), std::string()));
		if (!pchCompile.empty())
		{

			std::string compilerEcho;
			if (m_toolchain->type() != ToolchainType::MSVC)
			{
				compilerEcho = getCompileEchoSources(object) + "\n\t";
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
	auto rcCompile = String::join(m_toolchain->getRcCompileCommand(source, object, m_generateDependencies, dependency));
	if (!rcCompile.empty())
	{
		const auto compilerEcho = getCompileEchoSources(source);

		ret = fmt::format(R"makefile(
{object}: {source}
	{compilerEcho}
	{quietFlag}{rcCompile} 1>nul
)makefile",
			FMT_ARG(object),
			FMT_ARG(source),
			FMT_ARG(compilerEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(rcCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();

	std::string dependency;
	const auto specialization = m_project->language() == CodeLanguage::CPlusPlus ? CxxSpecialization::CPlusPlus : CxxSpecialization::C;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand(source, object, m_generateDependencies, dependency, specialization));
	if (!cppCompile.empty())
	{
		std::string compilerEcho;
		if (m_toolchain->type() != ToolchainType::MSVC)
		{
			compilerEcho = getCompileEchoSources(source) + "\n\t";
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
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
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

/*****************************************************************************/
std::string MakefileGeneratorNMake::getBuildColor() const
{
	auto color = Output::getAnsiStyleUnescaped(Output::theme().build);
	return fmt::format("\x1b[{}m", color);
}
}