/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/IntelCompileEnvironment.hpp"

namespace chalet
{
/*****************************************************************************/
IntelCompileEnvironment::IntelCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState) :
	CompileEnvironment(inInputs, inState)
{
}

/*****************************************************************************/
bool IntelCompileEnvironment::create(const std::string& inVersion)
{
	UNUSED(inVersion);

	LOG("hello, from IntelCompileEnvironment!");

	return true;
}
}
