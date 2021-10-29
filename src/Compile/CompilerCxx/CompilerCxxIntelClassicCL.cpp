/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxIntelClassicCL.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxIntelClassicCL::CompilerCxxIntelClassicCL(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxVisualStudioCL(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxIntelClassicCL::initialize()
{
	if (!createPrecompiledHeaderSource())
		return false;

	return true;
}

/*****************************************************************************/
void CompilerCxxIntelClassicCL::addIncludes(StringList& outArgList) const
{
	outArgList.push_back("/X");

	CompilerCxxVisualStudioCL::addIncludes(outArgList);
}

/*****************************************************************************/
void CompilerCxxIntelClassicCL::addDiagnosticsOption(StringList& outArgList) const
{
	// CompileToolchainVisualStudio::addDiagnosticsOption(outArgList);
	UNUSED(outArgList);
}

}
