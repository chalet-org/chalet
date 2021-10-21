/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_ENVIRONMENT_GNU_HPP
#define CHALET_COMPILE_ENVIRONMENT_GNU_HPP

#include "Compile/Environment/ICompileEnvironment.hpp"

namespace chalet
{
struct CompileEnvironmentGNU final : ICompileEnvironment
{
	explicit CompileEnvironmentGNU(const CommandLineInputs& inInputs, BuildState& inState);

	virtual ToolchainType type() const noexcept final;
};
}

#endif // CHALET_COMPILE_ENVIRONMENT_GNU_HPP
