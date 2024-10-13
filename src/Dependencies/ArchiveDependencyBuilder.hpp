/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Dependencies/IDependencyBuilder.hpp"

namespace chalet
{
struct CentralState;
struct ArchiveDependency;
class ExternalDependencyCache;

struct ArchiveDependencyBuilder final : public IDependencyBuilder
{
	ArchiveDependencyBuilder(CentralState& inCentralState, const ArchiveDependency& inDependency);

	virtual bool validateRequiredTools() const final;
	virtual bool resolveDependency(StringList& outChanged) final;

private:
	bool localPathShouldUpdate(const bool inDestinationExists);
	bool fetchDependency(const bool inDestinationExists);

	bool needsUpdate();
	bool updateDependencyCache();

	void displayFetchingMessageStart();

	bool extractZipFile(const std::string& inFilename, const std::string& inDestination) const;
	bool extractTarFile(const std::string& inFilename, const std::string& inDestination) const;
	std::string getTempDestination() const noexcept;
	std::string getOutputFile() const noexcept;
	std::string getArchiveHash(const std::string& inFilename) const;

	const ArchiveDependency& m_archiveDependency;

	std::string m_lastHash;
};
}
