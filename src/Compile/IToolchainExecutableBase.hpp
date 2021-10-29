/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ITOOLCHAIN_EXECUTABLE_BASE_HPP
#define CHALET_ITOOLCHAIN_EXECUTABLE_BASE_HPP

#include "Compile/ToolchainType.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;

struct IToolchainExecutableBase
{
	explicit IToolchainExecutableBase(const BuildState& inState, const SourceTarget& inProject);
	CHALET_DISALLOW_COPY_MOVE(IToolchainExecutableBase);
	virtual ~IToolchainExecutableBase() = default;

protected:
	virtual std::string getQuotedExecutablePath(const std::string& inExecutable) const final;
	virtual std::string getPathCommand(std::string_view inCmd, const std::string& inPath) const final;

	const BuildState& m_state;
	const SourceTarget& m_project;

	bool m_quotePaths = true;

	bool m_isMakefile = false;
	bool m_isNinja = false;
	bool m_isNative = false;
};
}

#endif // CHALET_ITOOLCHAIN_EXECUTABLE_BASE_HPP
