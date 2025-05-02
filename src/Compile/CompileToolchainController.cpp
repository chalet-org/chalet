/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileToolchainController.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
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
		if (compilerCxx == nullptr || !compilerCxx->initialize())
			return false;
	}

	const auto& windRes = inState.toolchain.compilerWindowsResource();
	if (!windRes.empty())
	{
		compilerWindowsResource = ICompilerWinResource::make(type, windRes, inState, m_project);
		if (compilerWindowsResource == nullptr || !compilerWindowsResource->initialize())
			return false;
	}

	archiver = IArchiver::make(type, inState.toolchain.archiver(), inState, m_project);
	if (archiver == nullptr || !archiver->initialize())
		return false;

	linker = ILinker::make(type, inState.toolchain.linker(), inState, m_project);
	if (linker == nullptr || !linker->initialize())
		return false;

	return true;
}

/*****************************************************************************/
void CompileToolchainController::setQuotedPaths(const bool inValue) noexcept
{
	if (compilerCxx)
		compilerCxx->setQuotedPaths(inValue);

	if (compilerWindowsResource)
		compilerWindowsResource->setQuotedPaths(inValue);

	if (archiver)
		archiver->setQuotedPaths(inValue);

	if (linker)
		linker->setQuotedPaths(inValue);
}

/*****************************************************************************/
void CompileToolchainController::setGenerateDependencies(const bool inValue) noexcept
{
	if (compilerCxx)
		compilerCxx->setGenerateDependencies(inValue);

	if (compilerWindowsResource)
		compilerWindowsResource->setGenerateDependencies(inValue);

	if (archiver)
		archiver->setGenerateDependencies(inValue);

	if (linker)
		linker->setGenerateDependencies(inValue);
}

/*****************************************************************************/
StringList CompileToolchainController::getOutputTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	chalet_assert(archiver != nullptr, "");
	chalet_assert(linker != nullptr, "");

	if (m_project.isStaticLibrary())
		return archiver->getCommand(outputFile, sourceObjs);
	else
		return linker->getCommand(outputFile, sourceObjs);
}
}
