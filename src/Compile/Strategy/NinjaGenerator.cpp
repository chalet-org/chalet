/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/NinjaGenerator.hpp"

#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
NinjaGenerator::NinjaGenerator(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain) :
	m_state(inState),
	m_project(inProject),
	m_toolchain(inToolchain)
{
	m_rules = {
		{ "rc", &NinjaGenerator::getRcRule },
		{ "cpp", &NinjaGenerator::getCppRule },
		{ "c", &NinjaGenerator::getCppRule },
		{ "cc", &NinjaGenerator::getCppRule },
		{ "m", &NinjaGenerator::getCppRule },
		{ "mm", &NinjaGenerator::getCppRule },
		{ "M", &NinjaGenerator::getCppRule },
	};
}

/*****************************************************************************/
std::string NinjaGenerator::getContents(const SourceOutputs& inOutputs, const std::string& cacheDir)
{
	const auto target = m_state.paths.getTargetFilename(m_project);

	const std::string rules = getRules(inOutputs.fileExtensions);
	const std::string buildRules = getBuildRules(inOutputs);

	auto objects = String::join(inOutputs.objectList, " ");
	auto assemblies = String::join(inOutputs.assemblyList, " ");

	//
	//
	//
	//
	// ==============================================================================
	std::string ninjaTemplate = fmt::format(R"ninja(
builddir = {cacheDir}
{rules}{buildRules}

build {target}: link {objects}

build makebuild: phony | {target}

build dumpasm: phony | {assemblies}

default makebuild
)ninja",
		FMT_ARG(cacheDir),
		FMT_ARG(rules),
		FMT_ARG(buildRules),
		FMT_ARG(objects),
		FMT_ARG(assemblies),
		FMT_ARG(target));

	return ninjaTemplate;
}

/*****************************************************************************/
std::string NinjaGenerator::getRules(const StringList& inExtensions)
{
	std::string rules = getPchRule();
	for (auto& ext : inExtensions)
	{
		if (m_rules.find(ext) == m_rules.end())
		{
			Diagnostic::error(fmt::format("Ninja rule not found for file extension: '{}'", ext));
			return std::string();
		}

#if !defined(CHALET_WIN32)
		if (String::equals(ext, "rc"))
			continue;
#endif

		rules += m_rules[ext](*this);
	}
	rules += getAsmRule(); // Very broken
	rules += getLinkRule();

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getBuildRules(const SourceOutputs& inOutputs)
{
	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	std::string rules = getPchBuildRule(pchTarget);
	rules += "\n";

	rules += getObjBuildRules(inOutputs.objectList, pchTarget);
	rules += "\n";

	rules += getAsmBuildRules(inOutputs.assemblyList); // Very broken

	return rules;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchRule()
{
	std::string ret;

	if (m_project.usesPch())
	{
		const auto& depDir = m_state.paths.depDir();
		const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));

		const auto pchCompile = m_toolchain->getPchCompileCommand("$in", "$out", dependency);

		ret = fmt::format(R"ninja(
rule pch
  deps = gcc
  depfile = {dependency}
  description = $in
  command = {pchCompile}
)ninja",
			FMT_ARG(dependency),
			FMT_ARG(pchCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getRcRule()
{
	std::string ret;

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));

	const auto rcCompile = m_toolchain->getRcCompileCommand("$in", "$out", dependency);

	ret = fmt::format(R"ninja(
rule rc
  deps = gcc
  depfile = {dependency}
  description = $in
  command = {rcCompile}
)ninja",
		FMT_ARG(dependency),
		FMT_ARG(rcCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getAsmRule()
{
	std::string ret;

	if (m_project.dumpAssembly())
	{
		std::string asmCompile = m_toolchain->getAsmGenerateCommand("$in", "$out");
#if defined(CHALET_WIN32)
		asmCompile = fmt::format("bash -c \"{}\"", asmCompile);
#endif

		ret = fmt::format(R"ninja(
rule asm
  description = $out
  command = {asmCompile}
)ninja",
			FMT_ARG(asmCompile));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getCppRule()
{
	std::string ret;

	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());

	const auto& depDir = m_state.paths.depDir();
	const auto dependency = fmt::format("{depDir}/$in.d", FMT_ARG(depDir));

	const auto cppCompile = m_toolchain->getCppCompileCommand("$in", "$out", dependency);

	ret = fmt::format(R"ninja(
rule cxx
  deps = gcc
  depfile = {dependency}
  description = $in
  command = {cppCompile}
)ninja",
		FMT_ARG(dependency),
		FMT_ARG(cppCompile));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getLinkRule()
{
	std::string ret;

	const auto& compilerConfig = m_state.compilers.getConfig(m_project.language());
	const auto pchTarget = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig.isClang());
	const auto targetBasename = m_state.paths.getTargetBasename(m_project);

	const auto linkerCommand = m_toolchain->getLinkerTargetCommand("$out", "$in", targetBasename);

	ret = fmt::format(R"ninja(
rule link
  description = Linking $out
  command = {linkerCommand}
)ninja",
		FMT_ARG(linkerCommand));

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getPchBuildRule(const std::string& pchTarget)
{
	std::string ret;

	if (m_project.usesPch())
	{
		const auto& pch = m_project.pch();

		ret = fmt::format(R"ninja(
build {pchTarget}: pch {pch}
)ninja",
			FMT_ARG(pchTarget),
			FMT_ARG(pch));
	}

	return ret;
}

/*****************************************************************************/
std::string NinjaGenerator::getObjBuildRules(const StringList& inObjects, const std::string& pchTarget)
{
	std::string ret;

	const auto& objDir = fmt::format("{}/", m_state.paths.objDir());

	std::string pchImplicitDep;
	if (m_project.usesPch())
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
		else if (String::endsWith(".res", source))
			source = source.substr(0, source.size() - 4);

		std::string rule = "cxx";
		if (String::endsWith(".rc", source))
			rule = "rc";

		ret += fmt::format("build {obj}: {rule} {source}{pchImplicitDep}\n",
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

	if (m_project.dumpAssembly())
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

			ret += fmt::format("build {asmFile}: asm {object}\n",
				FMT_ARG(asmFile),
				FMT_ARG(object));
		}
	}

	return ret;
}
}
