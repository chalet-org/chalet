/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/IToolchainExecutableBase.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
IToolchainExecutableBase::IToolchainExecutableBase(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool IToolchainExecutableBase::quotedPaths() const noexcept
{
	return m_quotedPaths;
}
void IToolchainExecutableBase::setQuotedPaths(const bool inValue) noexcept
{
	m_quotedPaths = inValue;
}

/*****************************************************************************/
bool IToolchainExecutableBase::generateDependencies() const noexcept
{
	return m_generateDependencies;
}
void IToolchainExecutableBase::setGenerateDependencies(const bool inValue) noexcept
{
	m_generateDependencies = inValue;
}

/*****************************************************************************/
i32 IToolchainExecutableBase::executableMatches(const std::string& exec, const char* toolId, const char* id, const bool typeMatches, const char* label, const i32 opts)
{
	const bool failTypeMismatch = (opts & MatchOpts::FailTypeMismatch) == MatchOpts::FailTypeMismatch;
	const bool onlyType = (opts & MatchOpts::OnlyType) == MatchOpts::OnlyType;
	const bool checkPrefix = (opts & MatchOpts::CheckPrefix) == MatchOpts::CheckPrefix;

	const bool isExpected = String::equals(id, exec) || (checkPrefix && String::startsWith(id, fmt::format("{}-", exec)));
	if (isExpected && (!onlyType || (onlyType && typeMatches)))
	{
		return 1;
	}
	else if (failTypeMismatch && (isExpected && !typeMatches))
	{
		Diagnostic::error("Expected '{}' as the {} for {}, but found a different toolchain type.", id, toolId, label);
		return 0;
	}
	else if (typeMatches && onlyType)
	{
		Diagnostic::error("Expected '{}' as the {} for {}, but found '{}'", id, toolId, label, exec);
		return 0;
	}

	return -1;
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getQuotedPath(const BuildState& inState, const std::string& inPath, const bool inForce)
{
	bool native = !inForce && inState.toolchain.strategy() == StrategyType::Native;
	if (native)
		return inPath;
	else
		return fmt::format("\"{}\"", inPath);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getQuotedPath(const std::string& inPath) const
{
	if (isNative())
		return inPath;
	else
		return fmt::format("\"{}\"", inPath);
}

/*****************************************************************************/
std::string IToolchainExecutableBase::getPathCommand(std::string_view inCmd, const std::string& inPath) const
{
	if (isNative())
		return fmt::format("{}{}", inCmd, inPath);
	else
		return fmt::format("{}\"{}\"", inCmd, inPath);
}

/*****************************************************************************/
void IToolchainExecutableBase::addDefinesToList(StringList& outArgList, const std::string& inPrefix) const
{
	bool isNative = this->isNative();
	for (auto& define : m_project.defines())
	{
		auto pos = define.find("=\"");
		if (!isNative && pos != std::string::npos && define.back() == '\"')
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

/*****************************************************************************/
// Note: this could change in modulestrategy, so we check it from a function
bool IToolchainExecutableBase::isNative() const noexcept
{
	return !m_quotedPaths && m_state.toolchain.strategy() == StrategyType::Native;
}
}
