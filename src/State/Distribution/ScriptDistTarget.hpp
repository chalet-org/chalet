/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Distribution/IDistTarget.hpp"
#include "State/ScriptType.hpp"

namespace chalet
{
struct ScriptDistTarget final : public IDistTarget
{
	explicit ScriptDistTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& file() const noexcept;
	void setFile(std::string&& inValue) noexcept;

	ScriptType scriptType() const noexcept;
	void setScriptTye(const ScriptType inType) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(std::string&& inValue) noexcept;

	const std::string& dependsOn() const noexcept;
	void setDependsOn(std::string&& inValue) noexcept;

private:
	std::string m_workingDirectory;
	std::string m_file;
	StringList m_arguments;

	std::string m_dependsOn;

	ScriptType m_scriptType = ScriptType::None;
};
}
