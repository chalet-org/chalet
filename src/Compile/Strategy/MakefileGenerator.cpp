/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/MakefileGenerator.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace
{
/*****************************************************************************/
const char* unicodeRightwardsTripleArrow()
{
#if defined(CHALET_WIN32)
	if (Environment::isBash())
		return "\\xE2\\x87\\x9B";
#endif

	return Unicode::rightwardsTripleArrow();
}
}
/*****************************************************************************/
MakefileGenerator::MakefileGenerator(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
	m_cleanOutput = m_state.environment.cleanOutput();
	m_generateDependencies = true;
}

/*****************************************************************************/
std::string MakefileGenerator::getContents(const SourceOutputs& inOutputs)
{
	const auto target = m_state.paths.getTargetFilename(m_project);

	const auto& depDir = m_state.paths.depDir();

	const auto dumpAsmRecipe = getDumpAsmRecipe();
	const auto assemblyRecipe = getAsmRecipe();
	const auto pchRecipe = getPchRecipe();
	const auto makePchRecipe = getMakePchRecipe();
	const auto targetRecipe = getTargetRecipe();

	std::string rcRecipe;
#if defined(CHALET_WIN32)
	for (auto& ext : String::filterIf({ "rc", "RC" }, inOutputs.fileExtensions))
	{
		rcRecipe += getRcRecipe(ext);
	}
#endif

	std::string fileRecipes;
	for (auto& ext : String::filterIf({ "cpp", "CPP", "cc", "CC", "cxx", "CXX", "c++", "C++", "c", "C" }, inOutputs.fileExtensions))
	{
		fileRecipes += getCppRecipe(ext);
	}

	for (auto& ext : String::filterIf({ "m", "M", "mm" }, inOutputs.fileExtensions))
	{
		fileRecipes += getObjcRecipe(ext);
	}

	const auto suffixes = String::getPrefixed(inOutputs.fileExtensions, ".");

#if defined(CHALET_WIN32)
	const auto shell = "cmd.exe";
#else
	const auto shell = "/bin/sh";
#endif
	const auto printer = getPrinter();

	//
	//
	//
	//
	// ==============================================================================
	std::string makefileTemplate = fmt::format(R"makefile(
.SUFFIXES:
.SUFFIXES: {suffixes}

SHELL := {shell}

makebuild: {target}
	@{printer}
.DELETE_ON_ERROR: makebuild
{dumpAsmRecipe}{makePchRecipe}{fileRecipes}{pchRecipe}{rcRecipe}{assemblyRecipe}{targetRecipe}

{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

include $(wildcard $(SOURCE_DEPS))
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
std::string MakefileGenerator::getCompileEchoAsm()
{
	const auto purple = getColorPurple();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}$@", purple), true);
	}
	else
	{
		printer = getPrinter(std::string(purple));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGenerator::getCompileEchoSources()
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}$<", blue), true);
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGenerator::getCompileEchoLinker()
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		const auto arrow = unicodeRightwardsTripleArrow();
		printer = getPrinter(fmt::format("{blue}{arrow}  Linking $@", FMT_ARG(blue), FMT_ARG(arrow)), true);
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGenerator::getDumpAsmRecipe()
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
std::string MakefileGenerator::getAsmRecipe()
{
	std::string ret;

	const bool dumpAssembly = m_project.dumpAssembly();
	if (dumpAssembly)
	{
		const auto quietFlag = getQuietFlag();
		const auto& asmDir = m_state.paths.asmDir();
		const auto& objDir = m_state.paths.objDir();
		const auto asmCompile = m_toolchain->getAsmGenerateCommand("'$<'", "'$@'");
		const auto compileEcho = getCompileEchoAsm();

		ret = fmt::format(R"makefile(
{asmDir}/%.o.asm: {objDir}/%.o
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
std::string MakefileGenerator::getMakePchRecipe()
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
std::string MakefileGenerator::getPchRecipe()
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
std::string MakefileGenerator::getRcRecipe(const std::string& ext)
{
	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.{ext}",
		FMT_ARG(ext),
		FMT_ARG(depDir));
	const auto moveDependencies = getMoveCommand(dependency + ".Td", dependency + ".d");

	auto rcCompile = String::join(m_toolchain->getRcCompileCommand("$<", "$@", m_generateDependencies, fmt::format("{}.Td", dependency)));
	if (m_generateDependencies)
	{
		rcCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{objDir}/%.{ext}.res: %.{ext}
{objDir}/%.{ext}.res: %.{ext} {depDir}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{rcCompile}
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(ext),
		FMT_ARG(depDir),
		FMT_ARG(pchPreReq),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(rcCompile),
		FMT_ARG(dependency));

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getCppRecipe(const std::string& ext)
{
	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.{ext}",
		FMT_ARG(depDir),
		FMT_ARG(ext));
	const auto moveDependencies = getMoveCommand(dependency + ".Td", dependency + ".d");

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$<", "$@", m_generateDependencies, fmt::format("{}.Td", dependency), specialization));
	if (m_generateDependencies)
	{
		cppCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{objDir}/%.{ext}.o: %.{ext}
{objDir}/%.{ext}.o: %.{ext} {pchTarget} {depDir}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{cppCompile}
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(depDir),
		FMT_ARG(ext),
		FMT_ARG(pchPreReq),
		FMT_ARG(pchTarget),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(cppCompile),
		FMT_ARG(dependency));

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getObjcRecipe(const std::string& ext)
{
	std::string ret;

	const bool objectiveC = String::equals(ext, "m") || String::equals(ext, "M"); // mm & M imply C++

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.{ext}",
		FMT_ARG(depDir),
		FMT_ARG(ext));
	const auto moveDependencies = getMoveCommand(dependency + ".Td", dependency + ".d");

	const auto specialization = objectiveC ? CxxSpecialization::ObjectiveC : CxxSpecialization::ObjectiveCpp;
	auto objcCompile = String::join(m_toolchain->getCxxCompileCommand("$<", "$@", m_generateDependencies, fmt::format("{}.Td", dependency), specialization));
	if (m_generateDependencies)
	{
		objcCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{objDir}/%.{ext}.o: %.{ext}
{objDir}/%.{ext}.o: %.{ext} {depDir}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{objcCompile}
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(depDir),
		FMT_ARG(ext),
		FMT_ARG(pchPreReq),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(objcCompile),
		FMT_ARG(dependency));

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getTargetRecipe()
{
	std::string ret;

	const auto quietFlag = getQuietFlag();

	const auto preReqs = getLinkerPreReqs();

	const auto linkerTarget = m_state.paths.getTargetFilename(m_project);
	const auto linkerTargetBase = m_state.paths.getTargetBasename(m_project);
	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand(linkerTarget, { "$(SOURCE_OBJS)" }, linkerTargetBase));
	const auto compileEcho = getCompileEchoLinker();
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

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getPchOrderOnlyPreReq()
{
	std::string ret;
	if (m_project.usesPch())
		ret = " | makepch";

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getLinkerPreReqs()
{
	std::string ret = "$(SOURCE_OBJS)";

	for (auto& project : m_state.projects)
	{
		if (List::contains(m_project.projectStaticLinks(), project->name()))
		{
			ret += " " + m_state.paths.getTargetFilename(*project);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getQuietFlag()
{
	return m_cleanOutput ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGenerator::getMoveCommand(std::string inInput, std::string inOutput)
{
	if (Environment::isBash())
		return fmt::format("mv -f {} {}", inInput, inOutput);
	else
	{
		return fmt::format("del /f /q \"$(subst /,\\\\,{inOutput})\" 2> nul && rename \"$(subst /,\\\\,{inInput})\" \"$(notdir {inOutput})\"", FMT_ARG(inInput), FMT_ARG(inOutput));
	}
}

/*****************************************************************************/
std::string MakefileGenerator::getPrinter(const std::string& inPrint, const bool inNewLine)
{
	if (!Environment::isBash() && inPrint == "\\n")
	{
		return "echo.";
	}

	if (inPrint.empty())
	{
		return Environment::isBash() ? "printf ''" : "prompt"; // This just needs to be a noop
	}

	if (Environment::isBash())
	{
		return fmt::format("printf '{}{}'", inPrint, inNewLine ? "\\n" : "");
	}
	else
	{
		return fmt::format("echo {}", inPrint);
	}
}

/*****************************************************************************/
std::string MakefileGenerator::getColorBlue()
{
#if defined(CHALET_WIN32)
	return Environment::isBash() ? "\\033[0;34m" : "\x1b[0;34m";
	// return u8"\033[0;34m";
#else
	return "\\033[0;34m";
#endif
}

/*****************************************************************************/
std::string MakefileGenerator::getColorPurple()
{
#if defined(CHALET_WIN32)
	return Environment::isBash() ? "\\033[0;35m" : "\x1b[0;35m";
	// return u8"\x1b[0;35m";
#else
	return "\\033[0;35m";
#endif
}

}