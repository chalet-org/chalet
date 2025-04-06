/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Target/IBuildTarget.hpp"

namespace chalet
{
struct ProcessBuildTarget final : public IBuildTarget
{
	explicit ProcessBuildTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;
	virtual const std::string& getHash() const final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

	const std::string& workingDirectory() const noexcept;
	void setWorkingDirectory(std::string&& inValue) noexcept;

	const StringList& dependsOn() const noexcept;
	void addDependsOn(StringList&& inList);
	void addDependsOn(std::string&& inValue);

private:
	std::string m_workingDirectory;
	std::string m_path;

	StringList m_arguments;
	StringList m_dependsOn;
};
}
