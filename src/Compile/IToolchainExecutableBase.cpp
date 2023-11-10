/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/IToolchainExecutableBase.hpp"

#include "Environment/ICompileEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
IToolchainExecutableBase::IToolchainExecutableBase(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
	m_quotedPaths = inState.toolchain.strategy() == StrategyType::Native;
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
	if (m_quotedPaths)
		return inPath;
	else
		return fmt::format("\"{}\"", inPath);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (m_quotedPaths)
		return fmt::format("{}{}", inCmd, inPath);
	else
		return fmt::format("{}\"{}\"", inCmd, inPath);
}

/*****************************************************************************/
void IToolchainExecutableBase::addDefinesToList(StringList& outArgList, const std::string& inPrefix) const
{
	for (auto& define : m_project.defines())
	{
		auto pos = define.find("=\"");
		if (!m_quotedPaths && pos != std::string::npos && define.back() == '\"')
		{
			std::string key = define.substr(0, pos);
			std::string value = define.substr(pos + 2, define.size() - (key.size() + 3));
			std::string def = fmt::format("{}=\\\"{}\\\"", key, value);
			List::addIfDoesNotExist(outArgList, inPrefix + def);
		}
		else
		{
			List::addIfDoesNotExist(outArgList, inPrefix + define);
		}
	}
}

}
