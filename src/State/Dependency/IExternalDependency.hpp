/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Dependency/ExternalDependencyType.hpp"

namespace chalet
{
struct IExternalDependency;
struct CentralState;
using ExternalDependency = Unique<IExternalDependency>;

struct IExternalDependency
{
	explicit IExternalDependency(const CentralState& inCentralState, const ExternalDependencyType inType);
	virtual ~IExternalDependency() = default;

	[[nodiscard]] static ExternalDependency make(const ExternalDependencyType inType, const CentralState& inCentralState);

	virtual bool initialize() = 0;
	virtual bool validate() = 0;

	ExternalDependencyType type() const noexcept;
	bool isGit() const noexcept;
	bool isLocal() const noexcept;
	bool isScript() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

	bool needsUpdate() const noexcept;
	void setNeedsUpdate(const bool inValue) noexcept;

protected:
	bool replaceVariablesInPathList(StringList& outList);

	const CentralState& m_centralState;

private:
	std::string m_name;

	ExternalDependencyType m_type;

	bool m_needsUpdate = false;
};

using ExternalDependencyList = std::vector<ExternalDependency>;
}
