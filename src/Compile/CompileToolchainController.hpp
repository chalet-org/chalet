/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Archiver/IArchiver.hpp"
#include "Compile/CompilerCxx/ICompilerCxx.hpp"
#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"
#include "Compile/Linker/ILinker.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct CompileToolchainController
{
	explicit CompileToolchainController(const SourceTarget& inProject);

	bool initialize(const BuildState& inState);

	void setQuotedPaths(const bool inValue) noexcept;

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
