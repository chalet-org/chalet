/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainMSVC.hpp"

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
	UNUSED(inputFile, outputFile, generateDependency, dependency, specialization);
	return {};
}

/*****************************************************************************/
StringList CompileToolchainMSVC::getLinkerTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs, outputFileBase);
	return {};
}

}
