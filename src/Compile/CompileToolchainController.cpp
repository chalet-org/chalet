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
CompileToolchainController::CompileToolchainController(const SourceTarget& inProject) :
	m_project(inProject)
{
}

/*****************************************************************************/
bool CompileToolchainController::initialize(const BuildState& inState)
{
	ToolchainType type = inState.environment->type();

	const auto& cxxPath = inState.toolchain.compilerCxx(m_project.language()).path;
	if (!cxxPath.empty())
	{
		compilerCxx = ICompilerCxx::make(type, cxxPath, inState, m_project);
		if (!compilerCxx->initialize())
			return false;
	}

	const auto& windRes = inState.toolchain.compilerWindowsResource();
	if (!windRes.empty())
	{
		compilerWindowsResource = ICompilerWinResource::make(type, windRes, inState, m_project);
		if (!compilerWindowsResource->initialize())
			return false;
	}

	m_archiver = IArchiver::make(type, inState.toolchain.archiver(), inState, m_project);

	m_linker = ILinker::make(type, inState.toolchain.linker(), inState, m_project);
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
