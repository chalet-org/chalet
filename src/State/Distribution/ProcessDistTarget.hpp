/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Distribution/IDistTarget.hpp"

namespace chalet
{
struct ProcessDistTarget final : public IDistTarget
{
	explicit ProcessDistTarget(const BuildState& inState);

	virtual bool initialize() final;
	virtual bool validate() final;

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
