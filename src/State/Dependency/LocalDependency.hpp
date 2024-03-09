/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Dependency/IExternalDependency.hpp"

namespace chalet
{
struct LocalDependency final : public IExternalDependency
{
	explicit LocalDependency(const CentralState& inCentralState);

	virtual bool initialize() final;
	virtual bool validate() final;

	virtual std::string getStateHash(const BuildState& inState) const final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

private:
	std::string m_path;
};
}
