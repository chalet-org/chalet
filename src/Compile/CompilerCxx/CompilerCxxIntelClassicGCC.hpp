/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_GCC_HPP
#define CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_GCC_HPP

#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"

namespace chalet
{
struct CompilerCxxIntelClassicGCC : public CompilerCxxGCC
{
	explicit CompilerCxxIntelClassicGCC(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() final;

	virtual StringList getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch) final;

protected:
	virtual StringList getWarningExclusions() const final;

private:
	StringList m_warningExclusions;

	std::string m_pchSource;
};
}

#endif // CHALET_COMPILER_EXECUTABLE_CXX_INTEL_CLASSIC_GCC_HPP
