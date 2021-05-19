/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorGNU.hpp"

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
MakefileGeneratorGNU::MakefileGeneratorGNU(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	m_cleanOutput = m_state.environment.cleanOutput();
	// m_generateDependencies = inToolchain->type() != ToolchainType::MSVC && !Environment::isContinuousIntegrationServer();
	m_generateDependencies = true;
}

/*****************************************************************************/
void MakefileGeneratorGNU::addProjectRecipes(const ProjectConfiguration& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
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
	$(NOOP)
.DELETE_ON_ERROR: build_{hash}
)makefile",
		fmt::arg("hash", m_hash),
		FMT_ARG(buildRecipes),
		FMT_ARG(target));

	m_targetRecipes.push_back(std::move(makeTemplate));

	m_toolchain = nullptr;
	m_project = nullptr;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getContents(const std::string& inPath) const
{
	UNUSED(inPath);

	const auto& depDir = m_state.paths.depDir();
	const auto suffixes = String::getPrefixed(m_fileExtensions, ".");
	const auto printer = getPrinter();

#if defined(CHALET_WIN32)
	const auto shell = "cmd.exe";
#else
	const auto shell = "/bin/sh";
#endif

	const auto deps = String::join(m_dependencies);

	auto recipes = String::join(m_targetRecipes);

	std::string ninjaTemplate = fmt::format(R"makefile(
.SUFFIXES:
.SUFFIXES: {suffixes}

SHELL := {shell}

NOOP := @{printer}
{recipes}
{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

-include $(wildcard {deps})

.DEFAULT:
	$(NOOP)
)makefile",
		FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(printer),
		FMT_ARG(recipes),
		FMT_ARG(depDir),
		FMT_ARG(deps));

	return ninjaTemplate;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getBuildRecipes(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	for (auto& ext : inOutputs.fileExtensions)
	{
		List::addIfDoesNotExist(m_fileExtensions, ext);
	}

	const auto& compilerConfig = m_state.compilers.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClang());

	std::string recipes = getPchRecipe(pchTarget);

	recipes += getObjBuildRecipes(inOutputs.objectList, pchTarget);

	for (auto& dep : inOutputs.dependencyList)
	{
		List::addIfDoesNotExist(m_dependencies, dep);
	}

	if (m_project->dumpAssembly())
	{
		auto assemblies = String::excludeIf(m_assemblies, inOutputs.assemblyList);
		if (!assemblies.empty())
		{
			recipes += getAsmBuildRecipes(assemblies);

			for (auto& assem : assemblies)
			{
				List::addIfDoesNotExist(m_assemblies, assem);
			}
		}
	}

	recipes += getTargetRecipe(inOutputs.target, inOutputs.objectListLinker);

	return recipes;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget)
{
	const auto objDir = fmt::format("{}/", m_state.paths.objDir());

	std::string ret;

	for (auto& obj : inObjects)
	{
		if (obj.empty())
			continue;

		std::string source = obj;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith(".res", source))
			source = source.substr(0, source.size() - 4);

		if (String::endsWith({ ".rc", ".RC" }, source))
		{
			ret += getRcRecipe(source, obj);
		}
		else if (String::endsWith({ ".m", ".M", ".mm" }, source))
		{
			ret += getObjcRecipe(source, obj);
		}
		else
		{
			ret += getCppRecipe(source, obj, pchTarget);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getAsmBuildRecipes(const StringList& inAssemblies)
{
	std::string ret;

	if (m_project->dumpAssembly())
	{
		const auto& asmDir = m_state.paths.asmDir();
		const auto& objDir = m_state.paths.objDir();

		for (auto& asmFile : inAssemblies)
		{
			if (asmFile.empty())
				continue;

			std::string object = asmFile;
			String::replaceAll(object, asmDir, objDir);

			if (String::endsWith(".asm", object))
			{
				object = object.substr(0, object.size() - 4);
			}

			ret += getAsmRecipe(object, asmFile);
		}

		ret += getDumpAsmRecipe(inAssemblies);
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEchoAsm(const std::string& assembly) const
{
	const auto purple = getColorPurple();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}{}", purple, assembly), true);
	}
	else
	{
		printer = getPrinter(std::string(purple));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEchoSources(const std::string& source) const
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		printer = getPrinter(fmt::format("   {}{}", blue, source), true);
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEchoLinker(const std::string& target) const
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
	{
		const auto arrow = unicodeRightwardsTripleArrow();
		printer = getPrinter(fmt::format("{blue}{arrow}  Linking {target}", FMT_ARG(blue), FMT_ARG(arrow), FMT_ARG(target)), true);
	}
	else
	{
		printer = getPrinter(std::string(blue));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getDumpAsmRecipe(const StringList& inAssemblies) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool dumpAssembly = m_project->dumpAssembly();
	if (dumpAssembly)
	{
		const auto assemblies = String::join(inAssemblies);
		ret = fmt::format(R"makefile(
asm_{hash}: {assemblies}
	$(NOOP)
.PHONY: dumpasm
)makefile",
			fmt::arg("hash", m_hash),
			FMT_ARG(assemblies));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getAsmRecipe(const std::string& object, const std::string& assembly) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool dumpAssembly = m_project->dumpAssembly();
	if (dumpAssembly)
	{
		const auto quietFlag = getQuietFlag();
		const auto asmCompile = m_state.tools.getAsmGenerateCommand(fmt::format("'{}'", object), fmt::format("'{}'", assembly));
		const auto compileEcho = getCompileEchoAsm(assembly);

		ret = fmt::format(R"makefile(
{assembly}: {object}
	{compileEcho}
	{quietFlag}{asmCompile}
)makefile",
			FMT_ARG(assembly),
			FMT_ARG(object),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(asmCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getPchRecipe(const std::string& pchTarget)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const bool usePch = m_project->usesPch();

	if (usePch && !List::contains(m_precompiledHeaders, m_project->pch()))
	{
		const auto quietFlag = getQuietFlag();
		const auto& depDir = m_state.paths.depDir();
		const auto& pch = m_project->pch();
		m_precompiledHeaders.push_back(pch);

		auto dependency = fmt::format("{}/{}", depDir, pch);

		const auto tempDependency = dependency + ".Td";
		dependency += ".d";

		const auto moveDependencies = getMoveCommand(tempDependency, dependency);
		const auto compileEcho = getCompileEchoSources(pch);

		auto pchCompile = String::join(m_toolchain->getPchCompileCommand(pch, pchTarget, m_generateDependencies, tempDependency));
		if (m_generateDependencies)
		{
			pchCompile += fmt::format(" && {}", moveDependencies);
		}

		ret = fmt::format(R"makefile(
{pchTarget}: {pch}
{pchTarget}: {pch} {dependency}
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
std::string MakefileGeneratorGNU::getRcRecipe(const std::string& source, const std::string& object) const
{
	// chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources(source);
	const auto pchPreReq = getPchOrderOnlyPreReq();

	auto dependency = object;
	String::replaceAll(dependency, objDir, depDir);

	const auto tempDependency = dependency + ".Td";
	dependency += ".d";

	const auto moveDependencies = getMoveCommand(tempDependency, dependency);

	auto rcCompile = String::join(m_toolchain->getRcCompileCommand(source, object, m_generateDependencies, tempDependency));
	if (m_generateDependencies)
	{
		rcCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{object}: {source}
{object}: {source} {dependency}{pchPreReq}
	{compileEcho}
	{quietFlag}{rcCompile}
)makefile",
		FMT_ARG(object),
		FMT_ARG(source),
		FMT_ARG(dependency),
		FMT_ARG(pchPreReq),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(rcCompile));

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources(source);
	const auto pchPreReq = getPchOrderOnlyPreReq();

	auto dependency = object;
	String::replaceAll(dependency, objDir, depDir);

	const auto tempDependency = dependency + ".Td";
	dependency += ".d";

	const auto moveDependencies = getMoveCommand(tempDependency, dependency);

	const auto specialization = m_project->language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand(source, object, m_generateDependencies, tempDependency, specialization));
	if (m_generateDependencies)
	{
		cppCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{object}: {source}
{object}: {source} {pchTarget} {dependency}{pchPreReq}
	{compileEcho}
	{quietFlag}{cppCompile}
)makefile",
		FMT_ARG(object),
		FMT_ARG(source),
		FMT_ARG(pchTarget),
		FMT_ARG(dependency),
		FMT_ARG(pchPreReq),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(cppCompile));

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getObjcRecipe(const std::string& source, const std::string& object) const
{
	std::string ret;

	const bool objectiveC = String::endsWith({ ".m", ".M" }, source); // mm & M imply C++

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources(source);
	const auto pchPreReq = getPchOrderOnlyPreReq();

	auto dependency = object;
	String::replaceAll(dependency, objDir, depDir);

	const auto tempDependency = dependency + ".Td";
	dependency += ".d";

	const auto moveDependencies = getMoveCommand(tempDependency, dependency);

	const auto specialization = objectiveC ? CxxSpecialization::ObjectiveC : CxxSpecialization::ObjectiveCpp;
	auto objcCompile = String::join(m_toolchain->getCxxCompileCommand(source, object, m_generateDependencies, tempDependency, specialization));
	if (m_generateDependencies)
	{
		objcCompile += fmt::format(" && {}", moveDependencies);
	}

	ret = fmt::format(R"makefile(
{object}: {source}
{object}: {source} {dependency}{pchPreReq}
	{compileEcho}
	{quietFlag}{objcCompile}
)makefile",
		FMT_ARG(object),
		FMT_ARG(source),
		FMT_ARG(dependency),
		FMT_ARG(pchPreReq),
		FMT_ARG(compileEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(objcCompile));

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

	const auto linkerTargetBase = m_state.paths.getTargetBasename(*m_project);
	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand(linkerTarget, objects, linkerTargetBase));
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

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getPchOrderOnlyPreReq() const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;
	// if (m_project->usesPch())
	// 	ret = " | makepch";

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getLinkerPreReqs(const StringList& objects) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret = String::join(objects);

	for (auto& project : m_state.projects)
	{
		if (List::contains(m_project->projectStaticLinks(), project->name()))
		{
			ret += " " + m_state.paths.getTargetFilename(*project);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getQuietFlag() const
{
	return m_cleanOutput ? "@" : "";
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getMoveCommand(std::string inInput, std::string inOutput) const
{
	if (Environment::isBash())
		return fmt::format("mv -f {} {}", inInput, inOutput);
	else
	{
		return fmt::format("del /f /q \"$(subst /,\\\\,{inOutput})\" 2> nul && rename \"$(subst /,\\\\,{inInput})\" \"$(notdir {inOutput})\"", FMT_ARG(inInput), FMT_ARG(inOutput));
	}
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getPrinter(const std::string& inPrint, const bool inNewLine) const
{
	if (!Environment::isBash() && inPrint == "\\n")
	{
		return "echo.";
	}

	if (inPrint.empty())
	{
		return Environment::isBash() ? ":" : "rem"; // This just needs to be a noop
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
std::string MakefileGeneratorGNU::getColorBlue() const
{
#if defined(CHALET_WIN32)
	// return Environment::isBash() ? "\033[0;34m" : "\x1b[0;34m";
	return "\x1b[0;34m";
#else
	return "\\033[0;34m";
#endif
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getColorPurple() const
{
#if defined(CHALET_WIN32)
	// return Environment::isBash() ? "\033[0;35m" : "\x1b[0;35m";
	return "\x1b[0;35m";
#else
	return "\\033[0;35m";
#endif
}

}