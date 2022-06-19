/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/ModuleStrategy/IModuleStrategy.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"

#include "Compile/CompileToolchainController.hpp"
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

	Diagnostic::errorAbort("Unimplemented StrategyType requested: {}", static_cast<int>(inType));
	return nullptr;
}

/*****************************************************************************/
StrategyType ICompileStrategy::type() const noexcept
{
	return m_type;
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
void ICompileStrategy::setSourceOutputs(const SourceTarget& inProject, Unique<SourceOutputs>&& inOutputs)
{
	const auto& name = inProject.name();
	m_outputs[name] = std::move(inOutputs);
}

/*****************************************************************************/
void ICompileStrategy::setToolchainController(const SourceTarget& inProject, CompileToolchain&& inToolchain)
{
	const auto& name = inProject.name();
	m_toolchains[name] = std::move(inToolchain);
}

/*****************************************************************************/
bool ICompileStrategy::doPreBuild()
{
	return true;
}

/*****************************************************************************/
bool ICompileStrategy::doPostBuild() const
{
	return true;
}

/*****************************************************************************/
bool ICompileStrategy::addProject(const SourceTarget& inProject)
{
	if (m_state.info.generateCompileCommands()) // TODO: Not available yet w/ modules
	{
		if (!addCompileCommands(inProject))
			return false;
	}

	auto& outputs = m_outputs.at(inProject.name());
	outputs.reset();

	auto& toolchain = m_toolchains.at(inProject.name());
	toolchain.reset();

	return true;
}

/*****************************************************************************/
bool ICompileStrategy::buildProjectModules(const SourceTarget& inProject)
{
	const auto& name = inProject.name();
	if (m_outputs.find(name) != m_outputs.end())
	{
		auto& outputs = m_outputs.at(name);
		auto& toolchain = m_toolchains.at(name);

		auto moduleStrategy = IModuleStrategy::make(m_state.environment->type(), m_state);
		if (!moduleStrategy->initialize())
			return false;

		if (!moduleStrategy->buildProject(inProject, std::move(outputs), std::move(toolchain)))
			return false;
	}

	UNUSED(inProject);

	return true;
}

/*****************************************************************************/
bool ICompileStrategy::addCompileCommands(const SourceTarget& inProject)
{
	const auto& name = inProject.name();
	if (m_outputs.find(name) != m_outputs.end())
	{
		auto& outputs = m_outputs.at(name);
		auto& toolchain = m_toolchains.at(name);
		return m_compileCommandsGenerator.addCompileCommands(toolchain, *outputs);
	}

	return true;
}

}
