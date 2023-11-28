/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"

namespace chalet
{
struct CompilerWinResourceGNUWindRes : public ICompilerWinResource
{
	explicit CompilerWinResourceGNUWindRes(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency) override;

protected:
	virtual void addIncludes(StringList& outArgList) const override;
	virtual void addDefines(StringList& outArgList) const override;
};
}
