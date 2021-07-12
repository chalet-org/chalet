/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/NinjaGenerator.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
NinjaGenerator::NinjaGenerator(const BuildState& inState) :
	IStrategyGenerator(inState)
{
	m_rules = {
		{ SourceType::CPlusPlus, &NinjaGenerator::getCppRule },
		{ SourceType::C, &NinjaGenerator::getCRule },
		{ SourceType::CxxPrecompiledHeader, &NinjaGenerator::getPchRule },
		{ SourceType::ObjectiveCPlusPlus, &NinjaGenerator::getObjcppRule },
		{ SourceType::ObjectiveC, &NinjaGenerator::getObjcRule },
		{ SourceType::WindowsResource, &NinjaGenerator::getRcRule },
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

	const auto& config = m_state.toolchain.getConfig(m_project->language());
	m_needsMsvcDepsPrefix |= config.isMsvc();

	const std::string rules = getRules(inOutputs.types);
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

	m_targetRecipes.emplace_back(std::move(ninjaTemplate));

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
builddir = {buildCache}
{msvcDepsPrefix}
{recipes}
build makebuild: phony

default makebuild
)ninja",
		fmt::arg("buildCache", inPath),
		FMT_ARG(msvcDepsPrefix),
		FMT_ARG(recipes));

	return ninjaTemplate;
}

/*****************************************************************************/
std::string NinjaGenerator::getDepFile(const std::string& inDependency)
{
	std::string ret;

	const auto& config = m_state.toolchain.getConfig(m_project->language());
	if (!config.isMsvc())
	{
		ret = fmt::format(R"ninja(
  depfile = {dependency})ninja",
			fmt::arg("dependency", inDependency));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRules(const SourceTypeList& inTypes)
{
	std::string rules;
	for (auto& type : inTypes)
	{
		if (m_rules.find(type) == m_rules.end())
			continue;

		rules += m_rules[type](*this);
	}

	rules += getLinkRule();

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getBuildRules(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	std::string rules;
	for (auto& group : inOutputs.groups)
	{
		if (group->type == SourceType::CxxPrecompiledHeader)
		{
			if (group->objectFile.empty())
				continue;

			rules += getPchBuildRule(group->objectFile);
		}
	}
	rules += getObjBuildRules(inOutputs.groups);

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

		const auto& compilerConfig = m_state.toolchain.getConfig(m_project->language());
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project, compilerConfig.isClangOrMsvc());

		// Have to pass in pchTarget here because MSVC's PCH compile command is wack
		const auto pchCompile = String::join(m_toolchain->getPchCompileCommand("$in", pchTarget, m_generateDependencies, dependency));

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
std::string NinjaGenerator::getCRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::C));

	ret = fmt::format(R"ninja(
rule cc_{hash}
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
std::string NinjaGenerator::getCppRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::CPlusPlus));

	ret = fmt::format(R"ninja(
rule cpp_{hash}
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

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::ObjectiveCPlusPlus));

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
		const auto& config = m_state.toolchain.getConfig(m_project->language());
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
std::string NinjaGenerator::getObjBuildRules(const SourceFileGroupList& inGroups)
{
	std::string ret;

	StringList pches;
	for (auto& group : inGroups)
	{
		if (group->type == SourceType::CxxPrecompiledHeader)
		{
			pches.push_back(group->objectFile);
		}
	}

	std::string pchImplicitDep;
	if (!pches.empty())
	{
		pchImplicitDep = fmt::format(" | {}", String::join(std::move(pches)));
	}

	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& object = group->objectFile;
		if (source.empty())
			continue;

		std::string rule;
		switch (group->type)
		{
			case SourceType::C:
				rule = "cc";
				break;
			case SourceType::CPlusPlus:
				rule = "cpp";
				break;
			case SourceType::ObjectiveC:
				rule = "objc";
				break;
			case SourceType::ObjectiveCPlusPlus:
				rule = "objcpp";
				break;
			case SourceType::WindowsResource:
				rule = "rc";
				break;

			case SourceType::CxxPrecompiledHeader:
			case SourceType::Unknown:
			default: {
				break;
			}
		}

		if (rule.empty())
			continue;

		ret += fmt::format("build {object}: {rule}_{hash} {source}{pchImplicitDep}\n",
			fmt::arg("hash", m_hash),
			FMT_ARG(object),
			FMT_ARG(rule),
			FMT_ARG(source),
			FMT_ARG(pchImplicitDep));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRuleDeps() const
{
#if defined(CHALET_WIN32)
	const auto& config = m_state.toolchain.getConfig(m_project->language());
	return config.isMsvc() ? "msvc" : "gcc";
#else
	return "gcc";
#endif
}
}
