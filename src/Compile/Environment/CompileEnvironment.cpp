/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Environment/CompileEnvironment.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildState.hpp"

#include "Compile/Environment/IntelCompileEnvironment.hpp"
#include "Compile/Environment/VisualStudioCompileEnvironment.hpp"

namespace chalet
{
/*****************************************************************************/
CompileEnvironment::CompileEnvironment(const CommandLineInputs& inInputs, BuildState& inState) :
	m_inputs(inInputs),
	m_state(inState)
{
#if !defined(CHALET_WIN32)
	UNUSED(m_inputs, m_state);
#endif
}

/*****************************************************************************/
[[nodiscard]] Unique<CompileEnvironment> CompileEnvironment::make(const ToolchainType inType, const CommandLineInputs& inInputs, BuildState& inState)
{
	if (inType == ToolchainType::MSVC)
	{
		return std::make_unique<VisualStudioCompileEnvironment>(inInputs, inState);
	}
	else if (inType == ToolchainType::IntelCompilerClassic || inType == ToolchainType::IntelOneApiDPCXX)
	{
		return std::make_unique<IntelCompileEnvironment>(inInputs, inState);
	}

	return std::make_unique<CompileEnvironment>(inInputs, inState);
}

/*****************************************************************************/
const std::string& CompileEnvironment::detectedVersion() const
{
	return m_detectedVersion;
}

/*****************************************************************************/
bool CompileEnvironment::create(const std::string& inVersion)
{
	UNUSED(inVersion);
	return true;
}
}
