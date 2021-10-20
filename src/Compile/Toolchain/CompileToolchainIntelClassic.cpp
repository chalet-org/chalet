/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainIntelClassic.hpp"

#include "Compile/CompilerConfig.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompileToolchainIntelClassic::CompileToolchainIntelClassic(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainGNU(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainIntelClassic::type() const noexcept
{
	return ToolchainType::IntelClassic;
}

/*****************************************************************************/
bool CompileToolchainIntelClassic::initialize()
{
	if (!CompileToolchainGNU::initialize())
		return false;

	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.cpp", objDir, pch);

		m_pchMinusLocation = String::getPathFilename(pch);

		if (!Commands::pathExists(m_pchSource))
		{
			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", m_pchMinusLocation)))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
StringList CompileToolchainIntelClassic::getPchCompileCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch)
{
	StringList ret;

	UNUSED(arch);

	if (m_config.compilerExecutable().empty())
		return ret;

	addExectuable(ret, m_config.compilerExecutable());

	if (generateDependency)
	{
		ret.emplace_back("-MT");
		ret.push_back(outputFile);
		ret.emplace_back("-MMD");
		ret.emplace_back("-MP");
		ret.emplace_back("-MF");
		ret.push_back(dependency);
	}

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::CPlusPlus : CxxSpecialization::C;
	addOptimizationOption(ret);
	addLanguageStandard(ret, specialization);
	addWarnings(ret);

	addLibStdCppCompileOption(ret, specialization);
	addPositionIndependentCodeOption(ret);
	addCompileOptions(ret);
	addObjectiveCxxRuntimeOption(ret, specialization);
	addDiagnosticColorOption(ret);
	addNoRunTimeTypeInformationOption(ret);
	addNoExceptionsOption(ret);
	addThreadModelCompileOption(ret);
	addArchitecture(ret);
	addArchitectureOptions(ret);
	addMacosMultiArchOption(ret, arch);

	addDebuggingInformationOption(ret);
	addProfileInformationCompileOption(ret);

	addDefines(ret);

	ret.emplace_back("-pch-create");
	ret.push_back(outputFile);

	addIncludes(ret);
	addMacosSysRootOption(ret);

	UNUSED(inputFile);

	ret.emplace_back("-o");
	ret.push_back(outputFile);

	ret.emplace_back("-c");
	ret.push_back(m_pchSource);

	return ret;
}

/*****************************************************************************/
void CompileToolchainIntelClassic::addWarnings(StringList& outArgList) const
{
	const std::string prefix{ "-W" };
	for (auto& warning : m_project.warnings())
	{
		if (String::equals(warning, "pedantic"))
			continue;

		std::string out;
		if (String::equals(warning, "pedantic-errors"))
			out = "-" + warning;
		else
			out = prefix + warning;

		// if (isFlagSupported(out))
		outArgList.emplace_back(std::move(out));
	}

	if (m_project.usesPch())
	{
		std::string invalidPch = prefix + "invalid-pch";
		// if (isFlagSupported(invalidPch))
		List::addIfDoesNotExist(outArgList, std::move(invalidPch));
	}
}

/*****************************************************************************/
void CompileToolchainIntelClassic::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
	{
		const auto& compilerConfig = m_state.toolchain.getConfig(m_project.language());
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project, compilerConfig);

		outArgList.emplace_back("-pch-use");
		outArgList.emplace_back(getPathCommand("", objDirPch));
	}
}

}
