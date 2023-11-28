/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

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

	bool quotedPaths() const noexcept;
	void setQuotedPaths(const bool inValue) noexcept;

	bool generateDependencies() const noexcept;
	void setGenerateDependencies(const bool inValue) noexcept;

protected:
	static i32 executableMatches(const std::string& exec, const char* toolId, const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch, const bool onlyType);

	template <typename T>
	[[nodiscard]] static Unique<T> makeTool(i32 result, const BuildState& inState, const SourceTarget& inProject);

	static std::string getQuotedPath(const BuildState& inState, const std::string& inPath, const bool inForce);

	virtual std::string getQuotedPath(const std::string& inPath) const final;
	virtual std::string getPathCommand(std::string_view inCmd, const std::string& inPath) const final;

	virtual void addDefinesToList(StringList& outArgList, const std::string& inPrefix) const final;

	const BuildState& m_state;
	const SourceTarget& m_project;

private:
	bool isNative() const noexcept;

	bool m_quotedPaths = false;
	bool m_generateDependencies = true;
};
}

#include "Compile/IToolchainExecutableBase.inl"
