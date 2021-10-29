/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_TOOLCHAIN_CONTROLLER_HPP
#define CHALET_COMPILE_TOOLCHAIN_CONTROLLER_HPP

#include "Compile/Archiver/IArchiver.hpp"
#include "Compile/CompilerCxx/ICompilerCxx.hpp"
#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"
#include "Compile/CxxSpecialization.hpp"
#include "Compile/Linker/ILinker.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct CompileToolchainController
{
	explicit CompileToolchainController(const BuildState& inState, const SourceTarget& inProject);

	bool initialize();

	StringList getOutputTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);

	Unique<ICompilerCxx> compilerCxx;
	Unique<ICompilerWinResource> compilerWindowsResource;

private:
	const BuildState& m_state;
	const SourceTarget& m_project;

	Unique<IArchiver> m_archiver;
	Unique<ILinker> m_linker;
};

using CompileToolchain = Unique<CompileToolchainController>;
}

#endif // CHALET_COMPILE_TOOLCHAIN_CONTROLLER_HPP
