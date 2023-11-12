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
	virtual std::string getHash() const final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

	const StringList& arguments() const noexcept;
	void addArguments(StringList&& inList);
	void addArgument(std::string&& inValue);

	const std::string& dependsOn() const noexcept;
	void setDependsOn(std::string&& inValue) noexcept;

private:
	std::string m_path;
	StringList m_arguments;

	std::string m_dependsOn;
};
}
