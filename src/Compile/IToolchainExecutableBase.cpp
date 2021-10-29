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
	m_quotePaths = m_state.toolchain.strategy() != StrategyType::Native;

	m_isMakefile = m_state.toolchain.strategy() == StrategyType::Makefile;
	m_isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	m_isNative = m_state.toolchain.strategy() == StrategyType::Native;
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getQuotedExecutablePath(const std::string& inExecutable) const
{
	if (m_isNative)
		return inExecutable;
	else
		return fmt::format("\"{}\"", inExecutable);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_quotePaths)
		return fmt::format("{}\"{}\"", inCmd, inPath);
	else
		return fmt::format("{}{}", inCmd, inPath);
}

}
