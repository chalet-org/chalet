/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/MakefileGenerator.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MakefileGenerator::MakefileGenerator(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
	m_cleanOutput = m_state.environment.cleanOutput();
}

/*****************************************************************************/
std::string MakefileGenerator::getContents(const SourceOutputs& inOutputs)
{
	const auto target = m_state.paths.getTargetFilename(m_project);
	const auto colorBlue = getBlueColor();

	const auto& depDir = m_state.paths.depDir();

	const auto dumpAsmRecipe = getDumpAsmRecipe();
	const auto assemblyRecipe = getAsmRecipe();
	const auto pchRecipe = getPchRecipe();
	const auto makePchRecipe = getMakePchRecipe();
	const auto targetRecipe = getTargetRecipe();

	std::string rcRecipe;
#if defined(CHALET_WIN32)
	for (auto ext : String::filterIf({ "rc", "RC" }, inOutputs.fileExtensions))
		rcRecipe = getRcRecipe();
#endif

	std::string cppRecipes;
	for (auto ext : String::filterIf({ "cpp", "CPP", "cc", "CC", "cxx", "CXX", "c++", "C++", "c", "C" }, inOutputs.fileExtensions))
	{
		cppRecipes += getCppRecipe(ext);
	}

	if (m_project.objectiveCxx())
	{
		for (auto ext : String::filterIf({ "m", "mm", "M" }, inOutputs.fileExtensions))
		{
			cppRecipes += getObjcRecipe(ext);
		}
	}

	const auto suffixes = String::getPrefixed(inOutputs.fileExtensions, ".");

#if defined(CHALET_WIN32)
	const auto shell = "cmd.exe";
#else
	const auto shell = "/bin/sh";
#endif

	//
	//
	//
	//
	// ==============================================================================
	std::string makefileTemplate = fmt::format(R"makefile(
.SUFFIXES:
.SUFFIXES: {suffixes}

SHELL = {shell}

makebuild: {target}
	{colorBlue}
.DELETE_ON_ERROR: makebuild
{dumpAsmRecipe}{makePchRecipe}{cppRecipes}{pchRecipe}{rcRecipe}{assemblyRecipe}{targetRecipe}

{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

include $(wildcard $(SOURCE_DEPS))
)makefile",
		FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(colorBlue),
		FMT_ARG(target),
		FMT_ARG(dumpAsmRecipe),
		FMT_ARG(makePchRecipe),
		FMT_ARG(cppRecipes),
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
std::string MakefileGenerator::getUnicodeLinkingCommand()
{
#if defined(CHALET_WIN32)
	return "printf '\\xE2\\x87\\x9B'";
#elif defined(CHALET_MACOS)
	return u8"printf \"\u21DB\"";
#else
	return u8"echo -n \"\u21DB\"";
#endif
}

/*****************************************************************************/
std::string MakefileGenerator::getColorCommand(const ushort inId)
{
	chalet_assert(inId >= 0 && inId <= 9, "Invalid value for tput setaf");

	const bool isBash = Environment::isBash();
	const bool hasTerm = Environment::hasTerm();

	return isBash && hasTerm ?
		"tput setaf " + std::to_string(inId) :
		isBash && !hasTerm ? "printf ''" : "echo|set /p=\"\"";
}

/*****************************************************************************/
std::string MakefileGenerator::getBlueColor()
{
	return fmt::format("@{}", getColorCommand(4));
}

/*****************************************************************************/
std::string MakefileGenerator::getMagentaColor()
{
	return fmt::format("@{}", getColorCommand(5));
}

/*****************************************************************************/
std::string MakefileGenerator::getQuietFlag()
{
	return m_cleanOutput ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGenerator::getMoveCommand()
{
	const bool isBash = Environment::isBash();

	return isBash ? "mv -f" : "move";
}

/*****************************************************************************/
std::string MakefileGenerator::getCompileEchoAsm()
{
	if (m_cleanOutput)
		return "@printf '   $@\\n'";

	return std::string();
}

/*****************************************************************************/
std::string MakefileGenerator::getCompileEchoSources()
{
	if (m_cleanOutput)
		return "\n\t@printf '   $<\\n'";

	return std::string();
}

/*****************************************************************************/
std::string MakefileGenerator::getCompileEchoLinker()
{
	if (m_cleanOutput)
	{
		const auto unicodeLinking = getUnicodeLinkingCommand();

		return fmt::format(u8"@{unicodeLinking} && printf '  Linking $@\\n'",
			FMT_ARG(unicodeLinking));
	}

	return std::string();
}

/*****************************************************************************/
std::string MakefileGenerator::getDumpAsmRecipe()
{
	std::string ret;

	const bool dumpAssembly = m_project.dumpAssembly();
	if (dumpAssembly)
	{
		const auto colorBlue = getBlueColor();

		ret = fmt::format(R"makefile(
dumpasm: $(SOURCE_ASMS)
	{colorBlue}
.PHONY: dumpasm
)makefile",
			FMT_ARG(colorBlue));
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
		const auto colorMagenta = getMagentaColor();
		const auto asmCompile = m_toolchain->getAsmGenerateCommand("'$<'", "'$@'");
		const auto compileEcho = getCompileEchoAsm();

		ret = fmt::format(R"makefile(
{asmDir}/%.o.asm: {objDir}/%.o
	{colorMagenta}
	{compileEcho}
	{quietFlag}{asmCompile}
)makefile",
			FMT_ARG(asmDir),
			FMT_ARG(objDir),
			FMT_ARG(colorMagenta),
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
		const auto colorBlue = getBlueColor();

		ret = fmt::format(R"makefile(
makepch: {pchTarget}
	{colorBlue}
.PHONY: makepch
)makefile",
			FMT_ARG(pchTarget),
			FMT_ARG(colorBlue));
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
		const auto colorBlue = getBlueColor();
		const auto mv = getMoveCommand();

		const auto dependency = fmt::format("{depDir}/{pch}",
			FMT_ARG(depDir),
			FMT_ARG(pch));

		const auto pchCompile = m_toolchain->getPchCompileCommand(pch, pchTarget, fmt::format("{}.Td", dependency));
		const auto compileEcho = getCompileEchoSources();

		ret = fmt::format(R"makefile(
{pchTarget}: {pch}
{pchTarget}: {pch} {dependency}.d
	{colorBlue}{compileEcho}
	{quietFlag}{pchCompile} && {mv} {dependency}.Td {dependency}.d
)makefile",
			FMT_ARG(pchTarget),
			FMT_ARG(pch),
			FMT_ARG(colorBlue),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(pchCompile),
			FMT_ARG(mv),
			FMT_ARG(dependency));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getRcRecipe()
{
	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto colorBlue = getBlueColor();
	const auto compileEcho = getCompileEchoSources();
	const auto mv = getMoveCommand();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.rc",
		FMT_ARG(depDir));

	const auto rcCompile = m_toolchain->getRcCompileCommand("$<", "$@", fmt::format("{}.Td", dependency));

	ret = fmt::format(R"makefile(
{objDir}/%.rc.res: %.rc
{objDir}/%.rc.res: %.rc {depDir}/%.rc.d{pchPreReq}
	{colorBlue}{compileEcho}
	{quietFlag}{rcCompile} && {mv} {dependency}.Td {dependency}.d
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(depDir),
		FMT_ARG(pchPreReq),
		FMT_ARG(colorBlue),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(rcCompile),
		FMT_ARG(mv),
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
	const auto colorBlue = getBlueColor();
	const auto compileEcho = getCompileEchoSources();
	const auto mv = getMoveCommand();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.{ext}",
		FMT_ARG(depDir),
		FMT_ARG(ext));

	const auto cppCompile = m_toolchain->getCppCompileCommand("$<", "$@", fmt::format("{}.Td", dependency));

	ret = fmt::format(R"makefile(
{objDir}/%.{ext}.o: %.{ext}
{objDir}/%.{ext}.o: %.{ext} {pchTarget} {depDir}/%.{ext}.d{pchPreReq}
	{colorBlue}{compileEcho}
	{quietFlag}{cppCompile} && {mv} {dependency}.Td {dependency}.d
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(depDir),
		FMT_ARG(ext),
		FMT_ARG(pchPreReq),
		FMT_ARG(pchTarget),
		FMT_ARG(colorBlue),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(cppCompile),
		FMT_ARG(mv),
		FMT_ARG(dependency));

	return ret;
}

/*****************************************************************************/
std::string MakefileGenerator::getObjcRecipe(const std::string& ext)
{
	std::string ret;

	const bool objectiveC = String::equals(ext, "m"); // mm & M imply C++

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto colorBlue = getBlueColor();
	const auto compileEcho = getCompileEchoSources();
	const auto mv = getMoveCommand();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	const auto dependency = fmt::format("{depDir}/$*.{ext}",
		FMT_ARG(depDir),
		FMT_ARG(ext));

	const std::string objcCompile = m_toolchain->getObjcppCompileCommand("$<", "$@", fmt::format("{}.Td", dependency), objectiveC);

	ret = fmt::format(R"makefile(
{objDir}/%.{ext}.o: %.{ext}
{objDir}/%.{ext}.o: %.{ext} {depDir}/%.{ext}.d{pchPreReq}
	{colorBlue}{compileEcho}
	{quietFlag}{objcCompile} && {mv} {dependency}.Td {dependency}.d
)makefile",
		FMT_ARG(objDir),
		FMT_ARG(depDir),
		FMT_ARG(ext),
		FMT_ARG(pchPreReq),
		FMT_ARG(colorBlue),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(objcCompile),
		FMT_ARG(mv),
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
	const auto linkerCommand = m_toolchain->getLinkerTargetCommand(linkerTarget, "$(SOURCE_OBJS)", linkerTargetBase);
	const auto compileEcho = getCompileEchoLinker();
	const auto colorBlue = getBlueColor();

	ret = fmt::format(R"makefile(
{linkerTarget}: {preReqs}
	{colorBlue}
	{compileEcho}
	{quietFlag}{linkerCommand}
	@printf '\n'
)makefile",
		FMT_ARG(linkerTarget),
		FMT_ARG(colorBlue),
		FMT_ARG(preReqs),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(linkerCommand));

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

	return ret;
}

}