/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Strategy/ICompileStrategy.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Generator/IStrategyGenerator.hpp"
#include "Compile/ModuleStrategy/IModuleStrategy.hpp"
#include "Process/Process.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"

#include "Compile/CompileToolchainController.hpp"
#include "Compile/Strategy/CompileStrategyMSBuild.hpp"
#include "Compile/Strategy/CompileStrategyMakefile.hpp"
#include "Compile/Strategy/CompileStrategyNative.hpp"
#include "Compile/Strategy/CompileStrategyNinja.hpp"
#include "Compile/Strategy/CompileStrategyXcodeBuild.hpp"

namespace chalet
{
/*****************************************************************************/
ICompileStrategy::ICompileStrategy(const StrategyType inType, BuildState& inState) :
	m_state(inState),
	m_compileCommandsGenerator(inState),
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
#if defined(CHALET_WIN32)
		case StrategyType::MSBuild:
			return std::make_unique<CompileStrategyMSBuild>(inState);
#elif defined(CHALET_MACOS)
		case StrategyType::XcodeBuild:
			return std::make_unique<CompileStrategyXcodeBuild>(inState);
#endif
		default:
			break;
	}

	Diagnostic::errorAbort("Unimplemented StrategyType requested: {}", static_cast<i32>(inType));
	return nullptr;
}

/*****************************************************************************/
StrategyType ICompileStrategy::type() const noexcept
{
	return m_type;
}

bool ICompileStrategy::isMSBuild() const noexcept
{
	return m_type == StrategyType::MSBuild;
}

bool ICompileStrategy::isXcodeBuild() const noexcept
{
	return m_type == StrategyType::XcodeBuild;
}

/*****************************************************************************/
bool ICompileStrategy::saveCompileCommands() const
{
	auto buildHashChanged = m_state.cache.file().buildHashChanged();
	if (buildHashChanged || m_filesUpdated || !m_compileCommandsGenerator.fileExists())
	{
		if (!m_compileCommandsGenerator.save())
			return false;
	}

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
bool ICompileStrategy::doFullBuild()
{
	return true;
}

/*****************************************************************************/
bool ICompileStrategy::buildProject(const SourceTarget& inProject)
{
#if defined(CHALET_MACOS)
	// generate dsym on mac
	if (m_state.environment->isAppleClang() && m_state.configuration.debugSymbols())
	{
		if (!generateDebugSymbolFiles(inProject))
			return false;
	}
#else
	UNUSED(inProject);
#endif
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
	if (!addCompileCommands(inProject))
		return false;

	auto& outputs = m_outputs.at(inProject.name());
	outputs.reset();

	auto& toolchain = m_toolchains.at(inProject.name());
	toolchain.reset();

	return true;
}

/*****************************************************************************/
bool ICompileStrategy::buildProjectModules(const SourceTarget& inProject)
{
	m_state.paths.setBuildDirectoriesBasedOnProjectKind(inProject);

	const auto& name = inProject.name();
	if (m_outputs.find(name) != m_outputs.end())
	{
		auto moduleStrategy = IModuleStrategy::make(m_state.environment->type(), m_state, m_compileCommandsGenerator);
		if (moduleStrategy == nullptr)
			return false;

		moduleStrategy->outputs = std::move(m_outputs.at(name));
		moduleStrategy->toolchain = std::move(m_toolchains.at(name));
		if (!moduleStrategy->buildProject(inProject))
			return false;
	}

	checkIfTargetWasUpdated(inProject);

	return true;
}

/*****************************************************************************/
void ICompileStrategy::checkIfTargetWasUpdated(const SourceTarget& inProject)
{
	auto& sourceCache = m_state.cache.file().sources();
	auto output = m_state.paths.getTargetFilename(inProject);
	if (sourceCache.fileChangedOrDoesNotExist(output))
	{
		m_filesUpdated = true;
	}
}

/*****************************************************************************/
bool ICompileStrategy::addCompileCommands(const SourceTarget& inProject)
{
	// TODO: Not available yet w/ modules
	if (m_state.info.generateCompileCommands())
	{
		const auto& name = inProject.name();
		if (m_outputs.find(name) != m_outputs.end())
		{
			auto& outputs = m_outputs.at(name);
			auto& toolchain = m_toolchains.at(name);
			return m_compileCommandsGenerator.addCompileCommands(toolchain, *outputs);
		}
	}

	return true;
}

#if defined(CHALET_MACOS)
/*****************************************************************************/
// Mac only for now
bool ICompileStrategy::generateDebugSymbolFiles(const SourceTarget& inProject) const
{
	auto& sourceCache = m_state.cache.file().sources();
	if (inProject.isExecutable() || inProject.isSharedLibrary())
	{
		auto filename = m_state.paths.getTargetFilename(inProject);
		auto dsym = fmt::format("{}.dSYM", filename);
		if (sourceCache.fileChangedOrDoesNotExist(filename, dsym))
		{
			if (m_dsymUtil.empty())
			{
				m_dsymUtil = Files::which("dsymutil");
				if (m_dsymUtil.empty())
					return true;
			}

			if (!Process::run({ m_dsymUtil, filename, "-o", dsym }))
			{
				Diagnostic::error("There was a problem generating: {}", dsym);
				return false;
			}
		}
	}

	return true;
}

#endif
}
