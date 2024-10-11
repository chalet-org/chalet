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

struct ArchiveDependencyExtractor
{
	ArchiveDependencyExtractor(CentralState& inCentralState);

	bool run(ArchiveDependency& dependency, StringList& outChanged);

private:
	bool localPathShouldUpdate(const ArchiveDependency& inDependency, const bool inDestinationExists);
	bool fetchDependency(const ArchiveDependency& inDependency, const bool inDestinationExists);

	bool needsUpdate(const ArchiveDependency& inDependency);
	bool updateDependencyCache(const ArchiveDependency& inDependency);

	void displayCheckingForUpdates(const std::string& inDestination);
	void displayFetchingMessageStart(const ArchiveDependency& inDependency);

	CentralState& m_centralState;
	ExternalDependencyCache& m_dependencyCache;

	std::string m_lastHash;
};
}
