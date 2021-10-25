/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/NinjaGenerator.hpp"

#include "Compile/CompilerConfig.hpp"
#include "Compile/CompilerConfigController.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
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
void NinjaGenerator::addProjectRecipes(const SourceTarget& inProject, const SourceOutputs& inOutputs, CompileToolchain& inToolchain, const std::string& inTargetHash)
{
	m_project = &inProject;
	m_toolchain = inToolchain.get();
	m_hash = inTargetHash;

	m_needsMsvcDepsPrefix |= m_state.compilers.isMsvc();

	const std::string rules = getRules(inOutputs.types);
	const std::string buildRules = getBuildRules(inOutputs);

	auto objects = String::join(inOutputs.objectListLinker);

	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (List::contains(inProject.projectStaticLinks(), project.name()))
			{
				objects += " " + m_state.paths.getTargetFilename(project);
			}
		}
	}

	const std::string keyword = m_project->isStaticLibrary() ? "archive" : "link";

	//
	//
	//
	//
	// ==============================================================================
	std::string ninjaTemplate = fmt::format(R"ninja({rules}{buildRules}
build {target}: {keyword}_{hash} {objects}

build build_{hash}: phony | {target}
)ninja",
		fmt::arg("hash", m_hash),
		FMT_ARG(keyword),
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

	if (!m_state.compilers.isMsvc())
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
	chalet_assert(m_project != nullptr, "");

	const bool objectiveCxx = m_project->objectiveCxx();

	std::string rules;
	for (auto& type : inTypes)
	{
		if (m_rules.find(type) == m_rules.end())
			continue;

		if (!objectiveCxx && (type == SourceType::ObjectiveC || type == SourceType::ObjectiveCPlusPlus))
			continue;

		rules += m_rules[type](*this);
	}

	rules += getLinkRule();
	rules += '\n';

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getBuildRules(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	std::string rules;

	if (m_project->usesPch())
	{
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
		rules += getPchBuildRule(pchTarget);
	}

	rules += getObjBuildRules(inOutputs.groups);

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchRule()
{
	std::string ret;

	const auto& objDir = m_state.paths.objDir();

	const auto& pch = m_project->pch();
	if (m_project->usesPch() && !List::contains(m_precompiledHeaders, fmt::format("{}/{}", objDir, pch)))
	{
		const auto deps = getRuleDeps();
		const auto& depDir = m_state.paths.depDir();
		const auto dependency = fmt::format("{}/$in.d", depDir);
		const auto depFile = getDepFile(dependency);

		const auto object = m_state.paths.getPrecompiledHeaderTarget(*m_project);

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(object);
			auto filename = String::getPathFilename(object);

			for (auto& arch : m_state.info.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);

				auto pchCompile = String::join(m_toolchain->getPchCompileCommand("$in", outObject, m_generateDependencies, dependency, arch));
				if (!pchCompile.empty())
				{
					String::replaceAll(pchCompile, outObject, "$out");
					String::replaceAll(pchCompile, pch, "$in");
					ret += fmt::format(R"ninja(
rule pch_{arch}_{hash}
  deps = {deps}{depFile}
  description = $in ({arch})
  command = {pchCompile}
)ninja",
						fmt::arg("hash", m_hash),
						FMT_ARG(arch),
						FMT_ARG(deps),
						FMT_ARG(depFile),
						FMT_ARG(pchCompile));
				}
			}
		}
		else
#endif
		{
			// Have to pass in object here because MSVC's PCH compile command is wack
			auto pchCompile = String::join(m_toolchain->getPchCompileCommand("$in", object, m_generateDependencies, dependency, std::string()));
			if (!pchCompile.empty())
			{
				String::replaceAll(pchCompile, object, "$out");
				String::replaceAll(pchCompile, pch, "$in");
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
		}
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
	if (!rcCompile.empty())
	{
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
	}

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
	if (!cppCompile.empty())
	{
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

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->getCxxCompileCommand("$in", "$out", m_generateDependencies, dependency, CxxSpecialization::CPlusPlus));
	if (!cppCompile.empty())
	{
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
	}

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
	if (!cppCompile.empty())
	{
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
	}

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
	if (!cppCompile.empty())
	{
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
	}

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

	if (!linkerCommand.empty())
	{
		const std::string description = m_project->isStaticLibrary() ? "Archiving" : "Linking";
		const std::string keyword = m_project->isStaticLibrary() ? "archive" : "link";

		ret = fmt::format(R"ninja(
rule {keyword}_{hash}
  description = {description} $out
  command = {linkerCommand}
)ninja",
			fmt::arg("hash", m_hash),
			FMT_ARG(keyword),
			FMT_ARG(description),
			FMT_ARG(linkerCommand));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchBuildRule(const std::string& pchTarget)
{
	std::string ret;

	const auto& objDir = m_state.paths.objDir();
	const auto& pch = m_project->pch();
	auto pchCache = fmt::format("{}/{}", objDir, pch);

	if (m_project->usesPch() && !List::contains(m_precompiledHeaders, pchCache))
	{
		m_precompiledHeaders.push_back(std::move(pchCache));

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			std::string lastArch;
			for (auto& arch : m_state.info.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);
				auto dependencies = pch;

				if (!lastArch.empty())
				{
					dependencies += fmt::format(" | {}_{}/{}", baseFolder, lastArch, filename);
				}
				ret += fmt::format(R"ninja(
build {outObject}: pch_{arch}_{hash} {dependencies}
)ninja",
					fmt::arg("hash", m_hash),
					FMT_ARG(arch),
					FMT_ARG(outObject),
					FMT_ARG(dependencies));

				lastArch = arch;
			}
		}
		else
#endif
		{
			ret += fmt::format(R"ninja(
build {pchTarget}: pch_{hash} {pch}
)ninja",
				fmt::arg("hash", m_hash),
				FMT_ARG(pchTarget),
				FMT_ARG(pch));
		}

#if defined(CHALET_WIN32)
		if (m_state.compilers.isMsvc())
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
	if (m_project->usesPch())
	{
		const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);
			auto& lastArch = m_state.info.universalArches().back();
			pches.push_back(fmt::format("{}_{}/{}", baseFolder, lastArch, filename));
		}
		else
#endif
		{
			pches.push_back(pchTarget);
		}
	}

	std::string pchImplicitDep;
	if (!pches.empty())
	{
		pchImplicitDep = fmt::format(" | {}", String::join(std::move(pches)));
	}

	const bool objectiveCxx = m_project->objectiveCxx();
	for (auto& group : inGroups)
	{
		const auto& source = group->sourceFile;
		const auto& object = group->objectFile;
		if (source.empty())
			continue;

		if (!objectiveCxx && (group->type == SourceType::ObjectiveC || group->type == SourceType::ObjectiveCPlusPlus))
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

		if (group->type != SourceType::WindowsResource)
		{
			ret += fmt::format("build {object}: {rule}_{hash} {source}{pchImplicitDep}\n",
				fmt::arg("hash", m_hash),
				FMT_ARG(object),
				FMT_ARG(rule),
				FMT_ARG(source),
				FMT_ARG(pchImplicitDep));
		}
		else
		{
			ret += fmt::format("build {object}: {rule}_{hash} {source}\n",
				fmt::arg("hash", m_hash),
				FMT_ARG(object),
				FMT_ARG(rule),
				FMT_ARG(source));
		}
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRuleDeps() const
{
#if defined(CHALET_WIN32)
	return m_state.compilers.isMsvc() ? "msvc" : "gcc";
#else
	return "gcc";
#endif
}
}
