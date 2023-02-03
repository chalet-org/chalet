/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_IEXTERNAL_DEPENDENCY_HPP
#define CHALET_IEXTERNAL_DEPENDENCY_HPP

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

	virtual bool validate() = 0;

	ExternalDependencyType type() const noexcept;
	bool isGit() const noexcept;

	const std::string& name() const noexcept;
	void setName(const std::string& inValue) noexcept;

protected:
	const CentralState& m_centralState;

private:
	std::string m_name;

	ExternalDependencyType m_type;
};

using ExternalDependencyList = std::vector<ExternalDependency>;
}

#endif // CHALET_IEXTERNAL_DEPENDENCY_HPP
