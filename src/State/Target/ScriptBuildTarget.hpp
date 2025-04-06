/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/ScriptType.hpp"
#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ScriptBuildTarget final : public IBuildTarget
{
	explicit ScriptBuildTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual const std::string& getHash() const final;

	const std::string& file() const noexcept;
	void setFile(std::string&& inValue) noexcept;

	ScriptType scriptType() const noexcept;
	void setScriptTye(const ScriptType inType) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(std::string&& inValue) noexcept;

	const StringList& dependsOn() const noexcept;
	void addDependsOn(StringList&& inList);
	void addDependsOn(std::string&& inValue);

	void setDependsOnSelf(const bool inValue);

private:
	std::string m_workingDirectory;
	std::string m_file;
	StringList m_arguments;

	StringList m_dependsOn;

	ScriptType m_scriptType = ScriptType::None;

	bool m_dependsOnSelf = false;
};
}
