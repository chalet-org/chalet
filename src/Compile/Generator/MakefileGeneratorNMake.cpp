/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorNMake.hpp"

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
MakefileGeneratorNMake::MakefileGeneratorNMake(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	m_cleanOutput = m_state.environment.cleanOutput();
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

	m_targetRecipes.push_back(std::move(makeTemplate));

	m_toolchain = nullptr;
	m_project = nullptr;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getContents(const std::string& inPath) const
{
	UNUSED(inPath);

	const auto& depDir = m_state.paths.depDir();
	const auto suffixes = String::getPrefixed(m_fileExtensions, ".");
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
.SUFFIXES: {suffixes}

SHELL = {shell}
{recipes}{depDirs}
)makefile",
		FMT_ARG(suffixes),
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
std::string MakefileGeneratorNMake::getCompileEchoAsm(const std::string& file) const
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
std::string MakefileGeneratorNMake::getCompileEchoSources(const std::string& file) const
{
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
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
	const auto blue = getColorBlue();
	std::string printer;

	if (m_cleanOutput)
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

	for (auto& ext : inOutputs.fileExtensions)
	{
		List::addIfDoesNotExist(m_fileExtensions, ext);
	}

	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);

	std::string recipes = getPchRecipe(pchTarget);

	recipes += getPchBuildRecipe(pchTarget);

	recipes += getObjBuildRecipes(inOutputs.objectList, pchTarget);

	/*for (auto& dep : inOutputs.dependencyList)
	{
		List::addIfDoesNotExist(m_dependencies, dep);
	}*/

	if (m_state.environment.dumpAssembly())
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
std::string MakefileGeneratorNMake::getPchBuildRecipe(const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool usesPch = m_project->usesPch();
	if (usesPch)
	{
		const std::string targets = pchTarget;
		ret = fmt::format(R"makefile(
pch_{hash}: {targets}
)makefile",
			fmt::arg("hash", m_hash),
			FMT_ARG(targets));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget)
{
	const auto objDir = fmt::format("{}/", m_state.paths.objDir());

	std::string ret;

	for (auto& obj : inObjects)
	{
		if (obj.empty())
			continue;

		std::string source = obj;
		String::replaceAll(source, objDir, "");

		// if (String::endsWith(".o", source))
		// 	source = source.substr(0, source.size() - 2);
		// else
		if (String::endsWith({ ".res", ".obj" }, source))
			source = source.substr(0, source.size() - 4);

		if (String::endsWith({ ".rc", ".RC" }, source))
		{
			ret += getRcRecipe(source, obj);
		}
		else if (String::endsWith({ ".m", ".M", ".mm" }, source))
		{
			continue;
		}
		else
		{
			ret += getCppRecipe(source, obj, pchTarget);
		}
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getAsmBuildRecipes(const StringList& inAssemblies)
{
	std::string ret;

	if (m_state.environment.dumpAssembly())
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
std::string MakefileGeneratorNMake::getTargetRecipe(const std::string& linkerTarget, const StringList& objects) const
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
std::string MakefileGeneratorNMake::getDumpAsmRecipe(const StringList& inAssemblies) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool dumpAssembly = m_state.environment.dumpAssembly();
	if (dumpAssembly)
	{
		const auto assemblies = String::join(inAssemblies);
		ret = fmt::format(R"makefile(
asm_{hash}: {assemblies}
)makefile",
			fmt::arg("hash", m_hash),
			FMT_ARG(assemblies));
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getAsmRecipe(const std::string& object, const std::string& assembly) const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;

	const bool dumpAssembly = m_state.environment.dumpAssembly();
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
std::string MakefileGeneratorNMake::getPchRecipe(const std::string& pchTarget)
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
		// const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());

		const auto dependency = fmt::format("{depDir}/{pch}",
			FMT_ARG(depDir),
			FMT_ARG(pch));

		auto pchCompile = String::join(m_toolchain->getPchCompileCommand(pch, pchTarget, m_generateDependencies, fmt::format("{}.Td", dependency)));

		const auto compilerEcho = getCompileEchoSources(pchTarget);

		ret = fmt::format(R"makefile(
{pchTarget}: {pch}
	{quietFlag}{pchCompile}
)makefile",
			FMT_ARG(pchTarget),
			FMT_ARG(pch),
			FMT_ARG(compilerEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(pchCompile));
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

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");

	UNUSED(pchTarget);

	std::string ret;

	const auto quietFlag = getQuietFlag();

	std::string dependency;
	const auto specialization = m_project->language() == CodeLanguage::CPlusPlus ? CxxSpecialization::Cpp : CxxSpecialization::C;
	auto cppCompile = String::join(m_toolchain->getCxxCompileCommand(source, object, m_generateDependencies, dependency, specialization));

	const auto compilerEcho = getCompileEchoSources(source);

	ret = fmt::format(R"makefile(
{object}: {source}
	{quietFlag}{cppCompile}
)makefile",
		FMT_ARG(source),
		FMT_ARG(compilerEcho),
		FMT_ARG(quietFlag),
		FMT_ARG(cppCompile),
		FMT_ARG(object));

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
	return m_cleanOutput ? "@" : "";
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
std::string MakefileGeneratorNMake::getColorBlue() const
{
	return "\x1b[0;34m";
}

/*****************************************************************************/
std::string MakefileGeneratorNMake::getColorPurple() const
{
	return "\x1b[0;35m";
}
#else
MakefileGeneratorNMake::MakefileGeneratorNMake()
{
}
#endif
}