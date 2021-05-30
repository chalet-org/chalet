/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/NinjaGenerator.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
NinjaGenerator::NinjaGenerator(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	m_rules = {
		{ "cpp", &NinjaGenerator::getCppRule },
		{ "CPP", &NinjaGenerator::getCppRule },
		{ "cc", &NinjaGenerator::getCppRule },
		{ "CC", &NinjaGenerator::getCppRule },
		{ "cxx", &NinjaGenerator::getCppRule },
		{ "CXX", &NinjaGenerator::getCppRule },
		{ "c++", &NinjaGenerator::getCppRule },
		{ "C++", &NinjaGenerator::getCppRule },
		{ "c", &NinjaGenerator::getCppRule },
		{ "C", &NinjaGenerator::getCppRule },
		{ "mm", &NinjaGenerator::getObjcppRule },
		{ "m", &NinjaGenerator::getObjcRule },
		{ "M", &NinjaGenerator::getObjcRule },
		{ "rc", &NinjaGenerator::getRcRule },
		{ "RC", &NinjaGenerator::getRcRule },
	};
	// m_generateDependencies = !Environment::isContinuousIntegrationServer();
	m_generateDependencies = true;
}

/*****************************************************************************/
void NinjaGenerator::addProjectRecipes(const ProjectTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();
	m_hash = inTargetHash;

	const auto& config = m_state.compilerTools.getConfig(m_project->language());
	m_needsMsvcDepsPrefix |= config.isMsvc();

	const std::string rules = getRules(inOutputs.fileExtensions);
	const std::string buildRules = getBuildRules(inOutputs);

	auto objects = String::join(inOutputs.objectListLinker);

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const ProjectTarget&>(*target);
			if (List::contains(inProject.projectStaticLinks(), project.name()))
			{
				objects += " " + m_state.paths.getTargetFilename(project);
			}
		}
	}

	//
	//
	//
	//
	// ==============================================================================
	std::string ninjaTemplate = fmt::format(R"ninja({rules}{buildRules}
build {target}: link_{hash} {objects}

build build_{hash}: phony | {target}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(rules),
		FMT_ARG(buildRules),
		FMT_ARG(objects),
		fmt::arg("target", inOutputs.target));

	//

	if (m_state.environment.dumpAssembly())
	{
		auto assemblies = String::join(inOutputs.assemblyList);
		ninjaTemplate += fmt::format(R"ninja(
build asm_{hash}: phony | {assemblies}
)ninja",
			fmt::arg("hash", m_hash),
			FMT_ARG(assemblies));
	}

	m_targetRecipes.push_back(std::move(ninjaTemplate));

	m_toolchain = nullptr;
	m_project = nullptr;
}

/*****************************************************************************/
std::string NinjaGenerator::getContents(const std::string& inPath) const
{

	auto recipes = String::join(m_targetRecipes);

	std::string msvcDepsPrefix;
#if defined(CHALET_WIN32)
	if (m_needsMsvcDepsPrefix)
	{
		msvcDepsPrefix = "msvc_deps_prefix = Note: including file:";
	}
#endif

	std::string ninjaTemplate = fmt::format(R"ninja(
builddir = {cacheDir}
{msvcDepsPrefix}
{recipes}
build makebuild: phony

default makebuild
)ninja",
		fmt::arg("cacheDir", inPath),
		FMT_ARG(msvcDepsPrefix),
		FMT_ARG(recipes));

	return ninjaTemplate;
}

/*****************************************************************************/
std::string NinjaGenerator::getDepFile(const std::string& inDependency)
{
	std::string ret;

	const auto& config = m_state.compilerTools.getConfig(m_project->language());
	if (!config.isMsvc())
	{
		ret = fmt::format(R"ninja(
  depfile = {dependency})ninja",
			fmt::arg("dependency", inDependency));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRules(const StringList& inExtensions)
{
	std::string rules = getPchRule();
	for (auto& inExt : inExtensions)
	{
		auto ext = String::toLowerCase(inExt);
		if (m_rules.find(ext) == m_rules.end())
		{
			continue;
		}

#if !defined(CHALET_WIN32)
		if (String::equals("rc", ext))
			continue;
#endif

		rules += m_rules[ext](*this);
	}

	rules += getAsmRule();
	rules += getLinkRule();

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getBuildRules(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	std::string rules = getPchBuildRule(pchTarget);
	rules += '\n';

	rules += getObjBuildRules(inOutputs.objectList, pchTarget);

	if (m_state.environment.dumpAssembly())
	{
		auto assemblies = String::excludeIf(m_assemblies, inOutputs.assemblyList);
		if (!assemblies.empty())
		{
			rules += getAsmBuildRules(assemblies);

			for (auto& assem : assemblies)
			{
				List::addIfDoesNotExist(m_assemblies, assem);
			}
		}
	}

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchRule()
{
	std::string ret;

	if (m_project->usesPch() && !List::contains(m_precompiledHeaders, m_project->pch()))
	{
		const auto deps = getRuleDeps();
		const auto& depDir = m_state.paths.depDir();
		const auto dependency = fmt::format("{}/$in.d", depDir);
		const auto depFile = getDepFile(dependency);

		const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

		// Have to pass in pchTarget here because MSVC's PCH compile command is wack
		const auto pchCompile = String::join(m_toolchain->getPchCompileCommand("$in", pchTarget, m_generateDependencies, dependency));

		// TODO: Unique to each PCH

		ret = fmt::format(R"ninja(
rule pch_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {pchCompile}
)ninja",
			fmt::arg("hash", m_hash),
			FMT_ARG(deps),
			FMT_ARG(depFile),
			FMT_ARG(pchCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRcRule()
{
	std::string ret;

	const auto deps = getRuleDeps();
	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto rcCompile = String::join(m_toolchain->getRcCompileCommand("$in", "$out", m_generateDependencies, dependency));

	ret = fmt::format(R"ninja(
rule rc_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {rcCompile}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(deps),
		FMT_ARG(depFile),
		FMT_ARG(rcCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getAsmRule()
{
	std::string ret;

	if (m_state.environment.dumpAssembly())
	{
		std::string asmCompile = m_state.tools.getAsmGenerateCommand("$in", "$out");

#if defined(CHALET_WIN32)
		if (!m_state.tools.bash().empty() && m_state.tools.bashAvailable())
			asmCompile = fmt::format("{} -c \"{}\"", m_state.tools.bash(), asmCompile);
#endif

		ret = fmt::format(R"ninja(
rule asm_{hash}
  description = $out
  command = {asmCompile}
)ninja",
			fmt::arg("hash", m_hash),
			FMT_ARG(asmCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getCppRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();
	// const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::Cpp));

	ret = fmt::format(R"ninja(
rule cxx_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {cppCompile}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(deps),
		FMT_ARG(depFile),
		FMT_ARG(cppCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getObjcRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();
	// const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::ObjectiveC));

	ret = fmt::format(R"ninja(
rule objc_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {cppCompile}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(deps),
		FMT_ARG(depFile),
		FMT_ARG(cppCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getObjcppRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();
	// const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	// const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::ObjectiveCpp));

	ret = fmt::format(R"ninja(
rule objcpp_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {cppCompile}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(deps),
		FMT_ARG(depFile),
		FMT_ARG(cppCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getLinkRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto& compilerConfig = m_state.compilerTools.getConfig(m_project->language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());
	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);

	const auto linkerCommand = String::join(m_toolchain->getLinkerTargetCommand("$out", { "$in" }, targetBasename));

	const std::string description = m_project->isStaticLibrary() ? "Archiving" : "Linking";

	ret = fmt::format(R"ninja(
rule link_{hash}
  description = {description} $out
  command = {linkerCommand}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(description),
		FMT_ARG(linkerCommand));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchBuildRule(const std::string& pchTarget)
{
	std::string ret;

	if (m_project->usesPch() && !List::contains(m_precompiledHeaders, m_project->pch()))
	{
		const auto& pch = m_project->pch();
		m_precompiledHeaders.push_back(pch);

		ret = fmt::format(R"ninja(
build {pchTarget}: pch_{hash} {pch}
)ninja",
			fmt::arg("hash", m_hash),
			FMT_ARG(pchTarget),
			FMT_ARG(pch));

#if defined(CHALET_WIN32)
		const auto& config = m_state.compilerTools.getConfig(m_project->language());
		if (config.isMsvc())
		{
			std::string pchObj = pchTarget;
			String::replaceAll(pchObj, ".pch", ".obj");

			ret += fmt::format(R"ninja(
build {pchObj}: phony {pchTarget}
)ninja",
				FMT_ARG(pchObj),
				FMT_ARG(pchTarget));
		}
#endif
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getObjBuildRules(const StringList& inObjects, const std::string& pchTarget)
{
	std::string ret;

	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	std::string pchImplicitDep;
	if (m_project->usesPch())
	{
		pchImplicitDep = fmt::format(" | {}", pchTarget);
	}

	for (auto& obj : inObjects)
	{
		if (obj.empty())
			continue;

		std::string source = obj;
		String::replaceAll(source, objDir, "");

		if (String::endsWith(".o", source))
			source = source.substr(0, source.size() - 2);
		else if (String::endsWith({ ".res", ".obj" }, source))
			source = source.substr(0, source.size() - 4);

		std::string rule = "cxx";
		if (String::endsWith({ ".rc", ".RC" }, source))
			rule = "rc";
		else if (String::endsWith({ ".m", ".M" }, source))
			rule = "objc";
		else if (String::endsWith(".mm", source))
			rule = "objcpp";

		ret += fmt::format("build {obj}: {rule}_{hash} {source}{pchImplicitDep}\n",
			fmt::arg("hash", m_hash),
			FMT_ARG(obj),
			FMT_ARG(rule),
			FMT_ARG(source),
			FMT_ARG(pchImplicitDep));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getAsmBuildRules(const StringList& inAssemblies)
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
				object = object.substr(0, object.size() - 4);

			ret += fmt::format("build {asmFile}: asm_{hash} {object}\n",
				fmt::arg("hash", m_hash),
				FMT_ARG(asmFile),
				FMT_ARG(object));
		}
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRuleDeps() const
{
#if defined(CHALET_WIN32)
	const auto& config = m_state.compilerTools.getConfig(m_project->language());
	return config.isMsvc() ? "msvc" : "gcc";
#else
	return "gcc";
#endif
}
}
