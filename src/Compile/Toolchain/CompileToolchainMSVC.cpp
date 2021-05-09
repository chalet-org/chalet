/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainMSVC.hpp"

#include "Libraries/Format.hpp"
#include "Libraries/Regex.hpp"
#include "Utility/String.hpp"

// https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options-listed-alphabetically?view=msvc-160

namespace chalet
{
/*****************************************************************************/
CompileToolchainMSVC::CompileToolchainMSVC(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	m_state(inState),
	m_project(inProject),
	m_config(inConfig),
	m_compilerType(m_config.compilerType())
{
	m_quotePaths = m_state.environment.strategy() != StrategyType::Native; // might not be needed
	UNUSED(m_project);
}

/*****************************************************************************/
ToolchainType CompileToolchainMSVC::type() const
{
	return ToolchainType::MSVC;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(inputFile, outputFile, generateDependency, dependency);
	return {};
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getRcCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(inputFile, outputFile, generateDependency, dependency);
	return {};
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getCxxCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	StringList ret;

	const auto& cc = m_config.compilerExecutable();
	ret.push_back(fmt::format("\"{}\"", cc));
	ret.push_back("/nologo");
	ret.push_back("/MP");

	addLanguageStandard(ret, specialization);
	ret.push_back("/EHsc");

	addDefines(ret);
	addIncludes(ret);

	if (!outputFile.empty())
	{
		ret.push_back("/Fo" + fmt::format("\"{}\"", outputFile));
	}
	else
	{
		ret.push_back("/Fo" + fmt::format("\"{}/\"", m_state.paths.objDir()));
	}

	ret.push_back("/c");
	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs, outputFileBase);
	return {};
}

/*****************************************************************************/
void CompileToolchainMSVC::addIncludes(StringList& inArgList) const
{
	// inArgList.push_back("/X"); // ignore "Path"

	const std::string prefix = "/I";

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		inArgList.push_back(fmt::format("{}\"{}\"", prefix, outDir));
	}

	for (const auto& dir : m_project.locations())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		inArgList.push_back(fmt::format("{}\"{}\"", prefix, outDir));
	}
}

/*****************************************************************************/
// void addLibDirs(StringList& inArgList);
// void addWarnings(StringList& inArgList);

/*****************************************************************************/
void CompileToolchainMSVC::addDefines(StringList& inArgList) const
{
	const std::string prefix = "/D";
	for (auto& define : m_project.defines())
	{
		inArgList.push_back(prefix + define);
	}
}

/*****************************************************************************/
void CompileToolchainMSVC::addLanguageStandard(StringList& inArgList, const CxxSpecialization specialization) const
{
	if (m_project.language() == CodeLanguage::C || specialization == CxxSpecialization::ObjectiveC)
	{
		std::string langStandard = String::toLowerCase(m_project.cStandard());
		if (String::equals(langStandard, "gnu2x")
			|| String::equals(langStandard, "gnu18")
			|| String::equals(langStandard, "gnu17")
			|| String::equals(langStandard, "c2x")
			|| String::equals(langStandard, "c18")
			|| String::equals(langStandard, "c17")
			|| String::equals(langStandard, "iso9899:2018")
			|| String::equals(langStandard, "iso9899:2017"))
		{
			inArgList.push_back("/std:c17");
		}
		else
		{
			inArgList.push_back("/std:c11");
		}
	}
	else
	{
		std::string langStandard = String::toLowerCase(m_project.cppStandard());
		if (String::equals(langStandard, "c++20")
			|| String::equals(langStandard, "c++2a")
			|| String::equals(langStandard, "gnu++20")
			|| String::equals(langStandard, "gnu++2a"))
		{
			inArgList.push_back("/std:c++latest");
		}
		else if (String::equals(langStandard, "c++17")
			|| String::equals(langStandard, "c++1z")
			|| String::equals(langStandard, "gnu++17")
			|| String::equals(langStandard, "gnu++1z"))
		{
			inArgList.push_back("/std:c++17");
		}
		else
		{
			inArgList.push_back("/std:c++14");
		}
	}
}

}
