/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxIntelClang.hpp"

#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxIntelClang::CompilerCxxIntelClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxClang(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxIntelClang::initialize()
{
	if (!CompilerCxxClang::initialize())
		return false;

	return true;
}

}
