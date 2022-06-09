/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Generator/NinjaGenerator.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Core/CommandLineInputs.hpp"
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

	m_needsMsvcDepsPrefix |= m_state.environment->isMsvc();

	const std::string rules = getRules(inOutputs);
	const std::string buildRules = getBuildRules(inOutputs);

	auto objects = String::join(inOutputs.objectListLinker);

	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (String::equals(project.name(), inProject.name()))
				break;

			if (List::contains(inProject.projectStaticLinks(), project.name()))
			{
				objects += " " + m_state.paths.getTargetFilename(project);
			}
		}
	}

	auto keyword = m_project->isStaticLibrary() ? "archive" : "link";

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
	// #if defined(CHALET_WIN32)
	/*if (m_needsMsvcDepsPrefix)
	{
		msvcDepsPrefix = "msvc_deps_prefix = Note: including file:";
	}*/
	// #endif

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

	if (!m_state.environment->isMsvc())
	{
		ret = fmt::format(R"ninja(
  depfile = {dependency})ninja",
			fmt::arg("dependency", inDependency));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRules(const SourceOutputs& inOutputs)
{
	chalet_assert(m_project != nullptr, "");

	const bool objectiveCxx = m_project->objectiveCxx();

	std::vector<SourceType> addedRules;

	std::string rules;
	for (auto& group : inOutputs.groups)
	{
		if (List::contains(addedRules, group->type))
			continue;

		if (m_rules.find(group->type) == m_rules.end())
			continue;

		if (!objectiveCxx && (group->type == SourceType::ObjectiveC || group->type == SourceType::ObjectiveCPlusPlus))
			continue;

		rules += m_rules[group->type](*this);
		addedRules.push_back(group->type);
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

	if (m_project->usesPrecompiledHeader())
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

	const auto& pch = m_project->precompiledHeader();
	if (m_project->usesPrecompiledHeader() && !List::contains(m_precompiledHeaders, fmt::format("{}/{}", objDir, pch)))
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

			for (auto& arch : m_state.inputs.universalArches())
			{
				auto outObject = fmt::format("{}_{}/{}", baseFolder, arch, filename);

				auto pchCompile = String::join(m_toolchain->compilerCxx->getPrecompiledHeaderCommand("$in", outObject, m_generateDependencies, dependency, arch));
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
			auto pchCompile = String::join(m_toolchain->compilerCxx->getPrecompiledHeaderCommand("$in", object, m_generateDependencies, dependency, std::string()));
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

	const auto rcCompile = String::join(m_toolchain->compilerWindowsResource->getCommand("$in", "$out", m_generateDependencies, dependency));
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
std::string NinjaGenerator::getCxxRule(const std::string inId, const CxxSpecialization inSpecialization)
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto deps = getRuleDeps();

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));
	const auto depFile = getDepFile(dependency);

	const auto cppCompile = String::join(m_toolchain->compilerCxx->getCommand("$in", "$out", m_generateDependencies, dependency, inSpecialization));
	if (!cppCompile.empty())
	{
		ret = fmt::format(R"ninja(
rule {id}_{hash}
  deps = {deps}{depFile}
  description = $in
  command = {cppCompile}
)ninja",
			fmt::arg("id", inId),
			fmt::arg("hash", m_hash),
			FMT_ARG(deps),
			FMT_ARG(depFile),
			FMT_ARG(cppCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getCRule()
{
	return getCxxRule("cc", CxxSpecialization::C);
}

/*****************************************************************************/
std::string NinjaGenerator::getCppRule()
{
	return getCxxRule("cpp", CxxSpecialization::CPlusPlus);
}

/*****************************************************************************/
std::string NinjaGenerator::getObjcRule()
{
	return getCxxRule("objc", CxxSpecialization::ObjectiveC);
}

/*****************************************************************************/
std::string NinjaGenerator::getObjcppRule()
{
	return getCxxRule("objcpp", CxxSpecialization::ObjectiveCPlusPlus);
}

/*****************************************************************************/
std::string NinjaGenerator::getLinkRule()
{
	chalet_assert(m_project != nullptr, "");
	chalet_assert(m_toolchain != nullptr, "");

	std::string ret;

	const auto targetBasename = m_state.paths.getTargetBasename(*m_project);
	const auto linkerCommand = String::join(m_toolchain->getOutputTargetCommand("$out", { "$in" }, targetBasename));

	if (!linkerCommand.empty())
	{
		const char* description = m_project->isStaticLibrary() ? "Archiving" : "Linking";
		const char* keyword = m_project->isStaticLibrary() ? "archive" : "link";

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
	const auto& pch = m_project->precompiledHeader();
	auto pchCache = fmt::format("{}/{}", objDir, pch);

	if (m_project->usesPrecompiledHeader() && !List::contains(m_precompiledHeaders, pchCache))
	{
		m_precompiledHeaders.push_back(std::move(pchCache));

#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);

			std::string lastArch;
			for (auto& arch : m_state.inputs.universalArches())
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
		if (m_state.environment->isMsvc())
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

	std::string pchImplicitDep;
	if (m_project->usesPrecompiledHeader())
	{
		StringList pches;
		auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(*m_project);
#if defined(CHALET_MACOS)
		if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			auto baseFolder = String::getPathFolder(pchTarget);
			auto filename = String::getPathFilename(pchTarget);
			auto& lastArch = m_state.inputs.universalArches().back();
			pches.push_back(fmt::format("{}_{}/{}", baseFolder, lastArch, filename));
		}
		else
#endif
		{
			pches.push_back(std::move(pchTarget));
		}
		if (!pches.empty())
		{
			auto deps = String::join(std::move(pches));
			pchImplicitDep = fmt::format(" | {}", deps);
		}
	}

	std::string configureFilesDeps;
	if (!m_project->configureFiles().empty())
	{
		auto deps = String::join(m_state.paths.getConfigureFiles(*m_project));
		configureFilesDeps = fmt::format(" | {}", deps);
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

		const auto& implicitDeps = group->type != SourceType::WindowsResource ? pchImplicitDep : configureFilesDeps;
		ret += fmt::format("build {object}: {rule}_{hash} {source}{implicitDeps}\n",
			fmt::arg("hash", m_hash),
			FMT_ARG(object),
			FMT_ARG(rule),
			FMT_ARG(source),
			FMT_ARG(implicitDeps));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRuleDeps() const
{
#if defined(CHALET_WIN32)
	return m_state.environment->isMsvc() ? "msvc" : "gcc";
#else
	return "gcc";
#endif
}
}
