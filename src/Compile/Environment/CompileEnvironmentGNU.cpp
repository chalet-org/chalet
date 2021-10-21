/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironmentGNU.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironmentGNU::CompileEnvironmentGNU(const CommandLineInputs& inInputs, BuildState& inState) :
	ICompileEnvironment(inInputs, inState)
{
}

/*****************************************************************************/
ToolchainType CompileEnvironmentGNU::type() const noexcept
{
	return ToolchainType::GNU;
}

}