/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_SCRIPT_BUILD_TARGET_HPP
#define CHALET_SCRIPT_BUILD_TARGET_HPP

#include "State/ScriptType.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ScriptBuildTarget final : public IBuildTarget
{
	explicit ScriptBuildTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual std::string getHash() const final;

	const std::string& file() const noexcept;
	void setFile(std::string&& inValue) noexcept;

	ScriptType scriptType() const noexcept;
	void setScriptTye(const ScriptType inType) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

private:
	std::string m_file;
	StringList m_arguments;

	ScriptType m_scriptType = ScriptType::None;
};
}

#endif // CHALET_SCRIPT_BUILD_TARGET_HPP
