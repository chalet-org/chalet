/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_INTEL_COMPILE_ENVIRONMENT_HPP
#define CHALET_INTEL_COMPILE_ENVIRONMENT_HPP

#include "Compile/Environment/CompileEnvironment.hpp"

namespace chalet
{
class IntelCompileEnvironment final : public CompileEnvironment
{
public:
	explicit IntelCompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState);

	virtual bool create(const std::string& inVersion = std::string()) final;

private:
	//
};
}

#endif // CHALET_INTEL_COMPILE_ENVIRONMENT_HPP
