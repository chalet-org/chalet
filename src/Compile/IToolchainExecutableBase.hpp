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
	static std::string getQuotedPath(const BuildState& inState, const std::string& inPath);

	virtual std::string getQuotedPath(const std::string& inPath) const final;
	virtual std::string getPathCommand(std::string_view inCmd, const std::string& inPath) const final;

	virtual void addDefinesToList(StringList& outArgList, const std::string& inPrefix) const final;

	const BuildState& m_state;
	const SourceTarget& m_project;

private:
	bool isNative() const noexcept;
};
}

#endif // CHALET_ITOOLCHAIN_EXECUTABLE_BASE_HPP
