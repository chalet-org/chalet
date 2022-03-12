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
	explicit CompileToolchainController(const SourceTarget& inProject);

	bool initialize(const BuildState& inState);

	StringList getOutputTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase);

	Unique<ICompilerCxx> compilerCxx;
	Unique<ICompilerWinResource> compilerWindowsResource;
	Unique<IArchiver> archiver;
	Unique<ILinker> linker;

private:
	const SourceTarget& m_project;
};

using CompileToolchain = Unique<CompileToolchainController>;
}

#endif // CHALET_COMPILE_TOOLCHAIN_CONTROLLER_HPP
