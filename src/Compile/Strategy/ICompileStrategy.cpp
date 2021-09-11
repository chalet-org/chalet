/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "Compile/Generator/IStrategyGenerator.hpp"

#include "Compile/Strategy/CompileStrategyMakefile.hpp"
#include "Compile/Strategy/CompileStrategyNative.hpp"
#include "Compile/Strategy/CompileStrategyNinja.hpp"

namespace chalet
{
/*****************************************************************************/
ICompileStrategy::ICompileStrategy(const StrategyType inType, BuildState& inState) :
	m_state(inState),
	m_type(inType)
{
	m_generator = IStrategyGenerator::make(inType, inState);
}

/*****************************************************************************/
[[nodiscard]] CompileStrategy ICompileStrategy::make(const StrategyType inType, BuildState& inState)
{
	switch (inType)
	{
		case StrategyType::Makefile:
			return std::make_unique<CompileStrategyMakefile>(inState);
		case StrategyType::Ninja:
			return std::make_unique<CompileStrategyNinja>(inState);
		case StrategyType::Native:
			return std::make_unique<CompileStrategyNative>(inState);
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented StrategyType requested: ", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
StrategyType ICompileStrategy::type() const noexcept
{
	return m_type;
}

/*****************************************************************************/
bool ICompileStrategy::addCompileCommands(const ProjectTarget& inProject, CompileToolchain& inToolchain)
{
	const auto& name = inProject.name();
	if (m_outputs.find(name) != m_outputs.end())
	{
		auto& outputs = m_outputs.at(name);
		return m_compileCommandsGenerator.addCompileCommands(inToolchain, outputs);
	}

	return true;
}

/*****************************************************************************/
bool ICompileStrategy::saveCompileCommands() const
{
	const auto& buildDir = m_state.paths.buildOutputDir();
	if (!m_compileCommandsGenerator.save(buildDir))
		return false;

	return true;
}

/*****************************************************************************/
const SourceOutputs& ICompileStrategy::getSourceOutput(const std::string& inTarget)
{
	chalet_assert(m_outputs.find(inTarget) != m_outputs.end(), "");
	return m_outputs.at(inTarget);
}

/*****************************************************************************/
bool ICompileStrategy::doPostBuild() const
{
	return true;
}
}
