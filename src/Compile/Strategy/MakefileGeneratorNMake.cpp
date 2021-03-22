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

	const auto dumpAsmRecipe = getDumpAsmRecipe();
	const auto assemblyRecipe = getAsmRecipe();
	const auto pchRecipe = getPchRecipe();
	const auto makePchRecipe = getMakePchRecipe();
	const auto targetRecipe = getTargetRecipe();

	std::string rcRecipe;
	for (auto& ext : String::filterIf({ "rc", "RC" }, inOutputs.fileExtensions))
	{
		rcRecipe += getRcRecipe(ext);
	}

	std::string fileRecipes;
	for (auto& ext : String::filterIf({ "cpp", "CPP", "cc", "CC", "cxx", "CXX", "c++", "C++", "c", "C" }, inOutputs.fileExtensions))
	{
		fileRecipes += getCppRecipe(ext);
	}

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

{dumpAsmRecipe}{makePchRecipe}{fileRecipes}{pchRecipe}{rcRecipe}{assemblyRecipe}{targetRecipe}
makebuild: {target}

{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

)makefile",
		FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(target),
		FMT_ARG(printer),
		FMT_ARG(dumpAsmRecipe),
		FMT_ARG(makePchRecipe),
		FMT_ARG(fileRecipes),
		FMT_ARG(pchRecipe),
		FMT_ARG(rcRecipe),
		FMT_ARG(assemblyRecipe),
		FMT_ARG(targetRecipe),
		FMT_ARG(depDir));

	// if (!isBash)
	// {
	// 	String::replaceAll(makefileTemplate, "/%)", "\\\\\\\\%)");
	// 	String::replaceAll(makefileTemplate, "/", "\\\\");
	// }

	return makefileTemplate;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoAsm()
{
	const auto purple = getColorPurple();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}$@", purple));
	}
	else
	{
		printer = getPrinter(std::string(purple));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoSources()
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}$<", blue));
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCompileEchoLinker()
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		const auto arrow = Unicode::rightwardsTripleArrow();
		printer = getPrinter(fmt::format("{blue}{arrow}  Linking $@", FMT_ARG(blue), FMT_ARG(arrow)));
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getDumpAsmRecipe()
{
	std::string ret;

	const bool dumpAssembly = m_project.dumpAssembly();
	const auto printer = getPrinter();
	if (dumpAssembly)
	{
		ret = fmt::format(R"makefile(
dumpasm: $(SOURCE_ASMS)
	@{printer}
.PHONY: dumpasm
)makefile",
			FMT_ARG(printer));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getAsmRecipe()
{
	std::string ret;

	const bool dumpAssembly = m_project.dumpAssembly();
	if (dumpAssembly)
	{
		const auto quietFlag = getQuietFlag();
		const auto& asmDir = m_state.paths.asmDir();
		const auto& objDir = m_state.paths.objDir();
		const auto asmCompile = m_state.tools.getAsmGenerateCommand("'$<'", "'$@'");
		const auto compileEcho = getCompileEchoAsm();

		ret = fmt::format(R"makefile(
{asmDir}/*.obj.asm: {objDir}/*.obj
	{compileEcho}
	{quietFlag}{asmCompile}
)makefile",
			FMT_ARG(asmDir),
			FMT_ARG(objDir),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(asmCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getMakePchRecipe()
{
	std::string ret;

	const bool usePch = m_project.usesPch();

	if (usePch)
	{
		const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
		const auto printer = getPrinter();

		ret = fmt::format(R"makefile(
makepch: {pchTarget}
	@{printer}
.PHONY: makepch
)makefile",
			FMT_ARG(pchTarget),
			FMT_ARG(printer));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPchRecipe()
{
	std::string ret;

	const bool usePch = m_project.usesPch();

	if (usePch)
	{
		const auto quietFlag = getQuietFlag();
		const auto& depDir = m_state.paths.depDir();
		const auto& pch = m_project.pch();
		const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());

		const auto dependency = fmt::format("{depDir}/{pch}",
			FMT_ARG(depDir),
			FMT_ARG(pch));

		const auto moveDependencies = getMoveCommand(dependency + ".Td", dependency + ".d");
		const auto compileEcho = getCompileEchoSources();

		auto pchCompile = String::join(m_toolchain->getPchCompileCommand(pch, pchTarget, m_generateDependencies, fmt::format("{}.Td", dependency)));
		if (m_generateDependencies)
		{
			pchCompile += fmt::format(" && {}", moveDependencies);
		}

		ret = fmt::format(R"makefile(
{pchTarget}: {pch}
{pchTarget}: {pch} {dependency}.d
	{compileEcho}
	{quietFlag}{pchCompile}
)makefile",
			FMT_ARG(pchTarget),
			FMT_ARG(pch),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(pchCompile),
			FMT_ARG(dependency));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getRcRecipe(const std::string& ext)
{
	std::string ret;

	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();

	std::string dependency;
	auto rcCompile = String::join(m_toolchain->getRcCompileCommand("$<", "$@", m_generateDependencies, dependency));

	for (auto& loc : m_project.locations())
	{
		ret += fmt::format(R"makefile(
{{{loc}}}.{ext}{{{objDir}}}.res ::
    {compileEcho}
	{rcCompile}
)makefile",
			FMT_ARG(loc),
			FMT_ARG(ext),
			FMT_ARG(objDir),
			FMT_ARG(compileEcho),
			FMT_ARG(rcCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCppRecipe(const std::string& ext)
{
	std::string ret;

	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	std::string dependency;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$<", "$@", m_generateDependencies, dependency, specialization));

	for (auto& loc : m_project.locations())
	{
		ret += fmt::format(R"makefile(
{{{loc}}}.{ext}{{{objDir}}}.obj ::
    {compileEcho}
	{cppCompile}
)makefile",
			FMT_ARG(loc),
			FMT_ARG(ext),
			FMT_ARG(objDir),
			FMT_ARG(compileEcho),
			FMT_ARG(cppCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getTargetRecipe()
{
	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& objDir = m_state.paths.objDir();

	const auto linkerTarget = m_state.paths.getTargetFilename(m_project);
	const auto linkerTargetBase = m_state.paths.getTargetBasename(m_project);
	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand(linkerTarget, { "$(SOURCE_OBJS)" }, linkerTargetBase));
	const auto compileEcho = getCompileEchoLinker();
	const auto printer = getPrinter("\\n");

	ret = fmt::format(R"makefile(
{linkerTarget}: $(SOURCE_OBJS)
	{compileEcho}
	{quietFlag}{linkerCommand}
	@{printer}
)makefile",
		FMT_ARG(linkerTarget),
		FMT_ARG(objDir),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(linkerCommand),
		FMT_ARG(printer));

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getPchOrderOnlyPreReq()
{
	std::string ret;
	if (m_project.usesPch())
		ret = " | makepch";

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getQuietFlag()
{
	return m_cleanOutput ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getMoveCommand(std::string inInput, std::string inOutput)
{
	return fmt::format("del /f /q \"$(subst /,\\\\,{inOutput})\" 2> nul && rename \"$(subst /,\\\\,{inInput})\" \"$(notdir {inOutput})\"", FMT_ARG(inInput), FMT_ARG(inOutput));
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