/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/MakefileGeneratorNMake.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
#if defined(CHALET_WIN32)
/*****************************************************************************/
MakefileGeneratorNMake::MakefileGeneratorNMake(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
	m_cleanOutput = m_state.environment.cleanOutput();
	m_generateDependencies = false;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getContents(const SourceOutputs& inOutputs)
{
	const auto target = m_state.paths.getTargetFilename(m_project);

	const auto& depDir = m_state.paths.depDir();

	const auto buildRecipes = getBuildRecipes(inOutputs);
	const auto objects = String::join(inOutputs.objectList);

	const auto suffixes = String::getPrefixed(inOutputs.fileExtensions, ".");

	const auto shell = "cmd.exe";
	const auto printer = getPrinter();

	//
	//
	//
	//
	// ==============================================================================
	std::string makefileTemplate = fmt::format(R"makefile(
.SUFFIXES:
.SUFFIXES: {suffixes}

SHELL = {shell}

{buildRecipes}{target}: {objects}

makebuild: {target}

{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

)makefile",
		FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(target),
		FMT_ARG(buildRecipes),
		FMT_ARG(objects),
		FMT_ARG(printer),
		FMT_ARG(depDir));

	// if (!isBash)
	// {
	// 	String::replaceAll(makefileTemplate, "/%)", "\\\\\\\\%)");
	// 	String::replaceAll(makefileTemplate, "/", "\\\\");
	// }

	return makefileTemplate;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoAsm(const std::string& file)
{
	const auto purple = getColorPurple();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {purple}{file}", FMT_ARG(purple), FMT_ARG(file)));
	}
	else
	{
		printer = getPrinter(std::string(purple));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoSources(const std::string& file)
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {blue}{file}", FMT_ARG(blue), FMT_ARG(file)));
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoLinker(const std::string& file)
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		const auto arrow = Unicode::rightwardsTripleArrow();
		printer = getPrinter(fmt::format("{blue}{arrow}  Linking {file}", FMT_ARG(blue), FMT_ARG(arrow), FMT_ARG(file)));
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
	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	std::string rules = getPchBuildRecipe(pchTarget);
	rules += '\n';

	rules += getObjBuildRecipes(inOutputs.objectList, pchTarget);
	rules += '\n';

	// rules += getAsmBuildRules(inOutputs.assemblyList); // Very broken

	return rules;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPchBuildRecipe(const std::string& pchTarget)
{
	UNUSED(pchTarget);
	return std::string();
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget)
{
	std::string ret;

	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	for (auto& obj : inObjects)
	{
		if (obj.empty())
			continue;

		std::string source = obj;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".obj", source) || String::endsWith(".res", source))
		{
			source = source.substr(0, source.size() - 4);
		}

		std::string rule = "cxx";
		if (String::endsWith(".rc", source) || String::endsWith(".RC", source))
		{
			ret += getRcRecipe(source, obj, pchTarget);
		}
		else
		{
			ret += getCppRecipe(source, obj, pchTarget);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getRcRecipe(const std::string& source, const std::string& object, const std::string& pchTarget)
{
	UNUSED(pchTarget);
	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto compileEcho = getCompileEchoSources(source);

	ret = fmt::format(R"makefile(
{object}: {source}
	{compileEcho}
	{quietFlag}rc /fo {object} {source} 1>nul
)makefile",
		FMT_ARG(source),
		FMT_ARG(quietFlag),
		FMT_ARG(compileEcho),
		FMT_ARG(object));

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget)
{
	UNUSED(pchTarget);
	std::string ret;

	const auto quietFlag = getQuietFlag();

	std::string dependency;
	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand(source, object, m_generateDependencies, dependency, specialization));

	ret = fmt::format(R"makefile(
{object}: {source}
	{quietFlag}{cppCompile}
)makefile",
		FMT_ARG(source),
		FMT_ARG(quietFlag),
		FMT_ARG(cppCompile),
		FMT_ARG(object));

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getQuietFlag()
{
	return m_cleanOutput ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPrinter(const std::string& inPrint)
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
std::string MakefileGeneratorNMake::getColorBlue()
{
	return "\x1b[0;34m";
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getColorPurple()
{
	return "\x1b[0;35m";
}
#else
MakefileGeneratorNMake::MakefileGeneratorNMake()
{
}
#endif
}