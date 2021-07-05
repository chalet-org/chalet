/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/MakefileGeneratorGNU.hpp"

#include "State/AncillaryTools.hpp"
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
		return "\\u21DB";
#endif

	return Unicode::rightwardsTripleArrow();
}
}

/*****************************************************************************/
MakefileGeneratorGNU::MakefileGeneratorGNU(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	// m_generateDependencies = inToolchain->type() != ToolchainType::MSVC && !Environment::isContinuousIntegrationServer();
	m_generateDependencies = true;
}

/*****************************************************************************/
void MakefileGeneratorGNU::addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
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

	m_targetRecipes.emplace_back(std::move(makeTemplate));

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

	auto recipes = String::join(m_targetRecipes);

	std::string makeTemplate = fmt::format(R"makefile(
.SUFFIXES:
.SUFFIXES: {suffixes}

SHELL := {shell}

NOOP := @{printer}{recipes}
{depDir}/%.d: ;
.PRECIOUS: {depDir}/%.d

-include $(DEPS_{hash})

.DEFAULT:
	$(NOOP)
)makefile",
		fmt::arg("hash", m_hash),
		FMT_ARG(suffixes),
		FMT_ARG(shell),
		FMT_ARG(printer),
		FMT_ARG(recipes),
		FMT_ARG(depDir));

	return makeTemplate;
}

/*****************************************************************************/
bool MakefileGeneratorGNU::saveDependencies() const
{
	// LOG("Save some stuffs?");
	return true;
}

/*****************************************************************************/
void MakefileGeneratorGNU::reset()
{
	m_targetRecipes.clear();
	m_fileExtensions.clear();
	m_locationCache.clear();
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getBuildRecipes(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	for (auto& ext : inOutputs.fileExtensions)
	{
		List::addIfDoesNotExist(m_fileExtensions, ext);
	}

	const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClang());

	std::string recipes = getPchRecipe(pchTarget);

#if defined(CHALET_WIN32)
	for (auto& ext : String::filterIf({ "rc", "RC" }, inOutputs.fileExtensions))
	{
		recipes += getRcRecipe(ext, pchTarget);
	}
#endif

	for (auto& ext : String::filterIf({ "cpp", "CPP", "cc", "CC", "cxx", "CXX", "c++", "C++", "c", "C" }, inOutputs.fileExtensions))
	{
		recipes += getCppRecipe(ext, pchTarget);
	}

	for (auto& ext : String::filterIf({ "m", "M", "mm" }, inOutputs.fileExtensions))
	{
		recipes += getObjcRecipe(ext);
	}

	recipes += getTargetRecipe(inOutputs.target);

	return recipes;
}

/*****************************************************************************/
/*std::string MakefileGeneratorGNU::getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget)
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
}*/

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEchoSources() const
{
	const auto color = getBuildColor();
	std::string printer;

	if (Output::cleanOutput())
	{
		printer = getPrinter(fmt::format("   {}$<", color), true);
	}
	else
	{
		printer = getPrinter(std::string(color));
	}

	return fmt::format("@{}", printer);
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCompileEchoLinker() const
{
	const auto color = getBuildColor();
	std::string printer;

	if (Output::cleanOutput())
	{
		const auto arrow = unicodeRightwardsTripleArrow();
		const std::string description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

		printer = getPrinter(fmt::format("{color}{arrow}  {description} $@", FMT_ARG(color), FMT_ARG(arrow), FMT_ARG(description)), true);
	}
	else
	{
		printer = getPrinter(std::string(color));
	}

	return fmt::format("@{}", printer);
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
		const auto compileEcho = getCompileEchoSources();

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
std::string MakefileGeneratorGNU::getRcRecipe(const std::string& ext, const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	for (auto& location : m_project->locations())
	{
		if (locationExists(location, ext))
			continue;

		auto dependency = fmt::format("{}/{}/$*.{}", depDir, location, ext);

		const auto tempDependency = dependency + ".Td";
		dependency += ".d";

		const auto moveDependencies = getMoveCommand(tempDependency, dependency);

		auto rcCompile = String::join(m_toolchain->getRcCompileCommand("$<", "$@", m_generateDependencies, tempDependency));
		if (m_generateDependencies && !m_state.toolchain.usingLlvmRC())
		{
			rcCompile += fmt::format(" && {}", moveDependencies);
		}

		ret += fmt::format(R"makefile(
{objDir}/{location}/%.{ext}.res: {location}/%.{ext}
{objDir}/{location}/%.{ext}.res: {location}/%.{ext} {pchTarget} {depDir}/{location}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{rcCompile}
)makefile",
			FMT_ARG(objDir),
			FMT_ARG(depDir),
			FMT_ARG(ext),
			FMT_ARG(location),
			FMT_ARG(pchTarget),
			FMT_ARG(dependency),
			FMT_ARG(pchPreReq),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(rcCompile));

		m_locationCache[location].push_back(ext);
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getCppRecipe(const std::string& ext, const std::string& pchTarget) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	for (auto& location : m_project->locations())
	{
		if (locationExists(location, ext))
			continue;

		auto dependency = fmt::format("{}/{}/$*.{}", depDir, location, ext);

		const auto tempDependency = dependency + ".Td";
		dependency += ".d";

		const auto moveDependencies = getMoveCommand(tempDependency, dependency);

		const auto specialization = m_project->language() == CodeLanguage::CPlusPlus ? CxxSpecialization::CPlusPlus : CxxSpecialization::C;
		auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$<", "$@", m_generateDependencies, tempDependency, specialization));
		if (m_generateDependencies)
		{
			cppCompile += fmt::format(" && {}", moveDependencies);
		}

		ret += fmt::format(R"makefile(
{objDir}/{location}/%.{ext}.o: {location}/%.{ext}
{objDir}/{location}/%.{ext}.o: {location}/%.{ext} {pchTarget} {depDir}/{location}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{cppCompile}
)makefile",
			FMT_ARG(objDir),
			FMT_ARG(depDir),
			FMT_ARG(ext),
			FMT_ARG(location),
			FMT_ARG(pchTarget),
			FMT_ARG(dependency),
			FMT_ARG(pchPreReq),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(cppCompile));

		m_locationCache[location].push_back(ext);
	}

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getObjcRecipe(const std::string& ext) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const bool objectiveC = String::equals({ "m", "M" }, ext); // mm & M imply C++

	const auto quietFlag = getQuietFlag();
	const auto& depDir = m_state.paths.depDir();
	const auto& objDir = m_state.paths.objDir();
	const auto compileEcho = getCompileEchoSources();
	const auto pchPreReq = getPchOrderOnlyPreReq();

	for (auto& location : m_project->locations())
	{
		if (locationExists(location, ext))
			continue;

		auto dependency = fmt::format("{}/{}/$*.{}", depDir, location, ext);

		const auto tempDependency = dependency + ".Td";
		dependency += ".d";

		const auto moveDependencies = getMoveCommand(tempDependency, dependency);

		const auto specialization = objectiveC ? CxxSpecialization::ObjectiveC : CxxSpecialization::ObjectiveCPlusPlus;
		auto objcCompile = String::join(m_toolchain->getCxxCompileCommand("$<", "$@", m_generateDependencies, tempDependency, specialization));
		if (m_generateDependencies)
		{
			objcCompile += fmt::format(" && {}", moveDependencies);
		}

		ret += fmt::format(R"makefile(
{objDir}/{location}/%.{ext}.o: {location}/%.{ext}
{objDir}/{location}/%.{ext}.o: {location}/%.{ext} {depDir}/{location}/%.{ext}.d{pchPreReq}
	{compileEcho}
	{quietFlag}{objcCompile}
)makefile",
			FMT_ARG(objDir),
			FMT_ARG(depDir),
			FMT_ARG(ext),
			FMT_ARG(location),
			FMT_ARG(dependency),
			FMT_ARG(pchPreReq),
			FMT_ARG(compileEcho),
			FMT_ARG(quietFlag),
			FMT_ARG(objcCompile));

		m_locationCache[location].push_back(ext);
	}

	return ret;
}

/*****************************************************************************/
bool MakefileGeneratorGNU::locationExists(const std::string& location, const std::string& ext) const
{
	bool found = false;
	for (auto& e : m_locationCache[location])
	{
		if (e == ext)
		{
			found = true;
			break;
		}
	}

	return found;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getTargetRecipe(const std::string& linkerTarget) const
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto quietFlag = getQuietFlag();

	const auto preReqs = getLinkerPreReqs();

	const auto linkerTargetBase = m_state.paths.getTargetBasename(*m_project);
	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand(linkerTarget, { fmt::format("$(OBJS_{})", m_hash) }, linkerTargetBase));
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
std::string MakefileGeneratorGNU::getPchOrderOnlyPreReq() const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret;
	// if (m_project->usesPch())
	// 	ret = " | makepch";

	return ret;
}

/*****************************************************************************/
std::string MakefileGeneratorGNU::getLinkerPreReqs() const
{
	chalet_assert(m_project != nullptr, "");

	std::string ret = fmt::format("$(OBJS_{})", m_hash);

	uint count = 0;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
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
std::string MakefileGeneratorGNU::getBuildColor() const
{
	auto color = Output::getAnsiStyleUnescaped(Output::theme().build);
#if defined(CHALET_WIN32)
	return fmt::format("\x1b[{}m", color);
#else
	return fmt::format("\\033[{}m", color);
#endif
}

}