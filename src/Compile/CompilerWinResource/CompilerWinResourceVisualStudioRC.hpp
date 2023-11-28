/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"

namespace chalet
{
struct CompilerWinResourceVisualStudioRC final : public ICompilerWinResource
{
	explicit CompilerWinResourceVisualStudioRC(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) final;

protected:
	virtual void addIncludes(StringList& outArgList) const final;
	virtual void addDefines(StringList& outArgList) const final;
};
}
