/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/IToolchainExecutableBase.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
IToolchainExecutableBase::IToolchainExecutableBase(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getQuotedPath(const BuildState& inState, const std::string& inPath)
{
	if (inState.toolchain.strategy() == StrategyType::Native)
		return inPath;
	else
		return fmt::format("\"{}\"", inPath);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getQuotedPath(const std::string& inPath) const
{
	if (m_state.toolchain.strategy() == StrategyType::Native)
		return inPath;
	else
		return fmt::format("\"{}\"", inPath);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_state.toolchain.strategy() != StrategyType::Native)
		return fmt::format("{}\"{}\"", inCmd, inPath);
	else
		return fmt::format("{}{}", inCmd, inPath);
}

}
