/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "State/Dependency/IExternalDependency.hpp"

namespace chalet
{
struct ArchiveDependency final : public IExternalDependency
{
	explicit ArchiveDependency(const CentralState& inCentralState);

	virtual bool initialize() final;
	virtual bool validate() final;

	virtual const std::string& getHash() const final;

	const std::string& url() const noexcept;
	void setUrl(std::string&& inValue) noexcept;

	const std::string& subdirectory() const noexcept;
	void setSubdirectory(std::string&& inValue) noexcept;

	const std::string& destination() const noexcept;

private:
	bool parseDestination();

	std::string m_url;
	std::string m_subdirectory;
	std::string m_destination;
};
}
