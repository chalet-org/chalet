/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LOCAL_DEPENDENCY_HPP
#define CHALET_LOCAL_DEPENDENCY_HPP

#include "State/Dependency/IExternalDependency.hpp"

namespace chalet
{
struct LocalDependency final : public IExternalDependency
{
	explicit LocalDependency(const CentralState& inCentralState);

	virtual bool initialize() final;
	virtual bool validate() final;

	const std::string& path() const noexcept;
	void setPath(std::string&& inValue) noexcept;

	bool needsUpdate() const noexcept;
	void setNeedsUpdate(const bool inValue) noexcept;

private:
	std::string m_path;

	bool m_needsUpdate = false;
};
}

#endif // CHALET_LOCAL_DEPENDENCY_HPP