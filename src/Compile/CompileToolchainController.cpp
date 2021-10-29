/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileToolchainController.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/ToolchainType.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
CompileToolchainController::CompileToolchainController(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool CompileToolchainController::initialize()
{
	ToolchainType type = m_state.environment->type();

	compilerCxx = ICompilerCxx::make(type, m_state.toolchain.compilerCxx(m_project.language()).path, m_state, m_project);
	compilerWindowsResource = ICompilerWinResource::make(type, m_state.toolchain.compilerWindowsResource(), m_state, m_project);
	m_archiver = IArchiver::make(type, m_state.toolchain.archiver(), m_state, m_project);
	m_linker = ILinker::make(type, m_state.toolchain.linker(), m_state, m_project);

	if (!compilerCxx->initialize())
		return false;

	if (!compilerWindowsResource->initialize())
		return false;

	if (!m_linker->initialize())
		return false;

	return true;
}

/*****************************************************************************/
StringList CompileToolchainController::getOutputTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	chalet_assert(m_archiver != nullptr, "");
	chalet_assert(m_linker != nullptr, "");

	if (m_project.kind() == ProjectKind::StaticLibrary)
		return m_archiver->getCommand(outputFile, sourceObjs, outputFileBase);
	else
		return m_linker->getCommand(outputFile, sourceObjs, outputFileBase);
}
}
