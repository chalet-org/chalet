/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"

namespace chalet
{
struct CompilerCxxIntelClassicGCC : public CompilerCxxGCC
{
	explicit CompilerCxxIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependency, const std::string& arch) final;

protected:
	virtual StringList getWarningExclusions() const final;

	virtual void addPchInclude(StringList& outArgList, const SourceType derivative) const final;
	virtual void addCharsets(StringList& outArgList) const final;
	virtual void addFastMathOption(StringList& outArgList) const final;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const final;

private:
	StringList m_warningExclusions;

	std::string m_pchSource;
};
}
