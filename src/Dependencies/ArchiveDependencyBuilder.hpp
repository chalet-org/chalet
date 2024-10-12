/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

namespace chalet
{
struct CentralState;
struct ArchiveDependency;
class ExternalDependencyCache;

struct ArchiveDependencyBuilder
{
	ArchiveDependencyBuilder(CentralState& inCentralState, const ArchiveDependency& inDependency);

	bool run(StringList& outChanged);

private:
	bool localPathShouldUpdate(const bool inDestinationExists);
	bool fetchDependency(const bool inDestinationExists);

	bool needsUpdate();
	bool updateDependencyCache();

	void displayCheckingForUpdates(const std::string& inDestination);
	void displayFetchingMessageStart();

	bool validateTools() const;
	bool extractZipFile(const std::string& inFilename, const std::string& inDestination) const;
	bool extractTarFile(const std::string& inFilename, const std::string& inDestination) const;
	std::string getTempDestination() const noexcept;
	std::string getOutputFile() const noexcept;
	std::string getArchiveHash(const std::string& inFilename) const;

	CentralState& m_centralState;
	const ArchiveDependency& m_archiveDependency;
	ExternalDependencyCache& m_dependencyCache;

	std::string m_lastHash;
};
}
