/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/ArchiveDependencyExtractor.hpp"

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "State/CentralState.hpp"
#include "State/Dependency/ArchiveDependency.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Hash.hpp"
#include "Utility/String.hpp"
#include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiveDependencyExtractor::ArchiveDependencyExtractor(CentralState& inCentralState) :
	m_centralState(inCentralState),
	m_dependencyCache(m_centralState.cache.file().externalDependencies())
{
}

/*****************************************************************************/
bool ArchiveDependencyExtractor::run(ArchiveDependency& dependency, StringList& outChanged)
{
	bool destinationExists = Files::pathExists(dependency.destination());
	if (!localPathShouldUpdate(dependency, destinationExists))
		return true;

	destinationExists = Files::pathExists(dependency.destination());
	if (fetchDependency(dependency, destinationExists))
	{
		if (!updateDependencyCache(dependency))
		{
			Diagnostic::error("Error fetching archive dependency: {}", dependency.name());
			return false;
		}

		outChanged.emplace_back(Files::getCanonicalPath(dependency.destination()));
		return true;
	}
	else
	{
		const auto& destination = dependency.destination();

		if (Files::pathExists(destination))
			Files::removeRecursively(destination);
	}

	Diagnostic::error("Error fetching archive dependency: {}", dependency.name());
	return false;
}

/*****************************************************************************/
bool ArchiveDependencyExtractor::localPathShouldUpdate(const ArchiveDependency& inDependency, const bool inDestinationExists)
{
	const auto& destination = inDependency.destination();
	if (!m_dependencyCache.contains(destination))
	{
		if (inDestinationExists)
			return Files::removeRecursively(destination);

		return true;
	}

	if (!inDestinationExists)
		return true;

	return needsUpdate(inDependency);
}

/*****************************************************************************/
bool ArchiveDependencyExtractor::fetchDependency(const ArchiveDependency& inDependency, const bool inDestinationExists)
{
	if (inDestinationExists && m_dependencyCache.contains(inDependency.destination()))
		return true;

	displayFetchingMessageStart(inDependency);

	const auto& destination = inDependency.destination();
	const auto& url = inDependency.url();
	const auto& subdirectory = inDependency.subdirectory();

	auto curl = Files::which("curl");
	if (curl.empty())
	{
		Diagnostic::error("archive dependency requires curl: {}", inDependency.name());
		return false;
	}
	auto openssl = Files::which("openssl");
	if (openssl.empty())
	{
		Diagnostic::error("archive dependency requires openssl: {}", inDependency.name());
		return false;
	}
	auto unzip = Files::which("unzip");
	if (unzip.empty())
	{
		Diagnostic::error("archive dependency requires unzip: {}", inDependency.name());
		return false;
	}

	Files::makeDirectory(destination);

	auto filename = String::getPathFilename(url);
	auto outputFile = fmt::format("{}/{}", destination, filename);
	if (!Process::run({ curl, "-s", "-L", "-o", outputFile, url }))
		return false;

	auto shaOutput = Process::runOutput({ openssl, "sha1", outputFile });
	auto hash = Hash::string(shaOutput);

	if (!subdirectory.empty())
	{
		auto d2 = fmt::format("{}/tmp", destination);
		if (!Process::runNoOutput({ unzip, outputFile, "-d", d2 }))
			return false;

		auto sub = fmt::format("{}/{}", d2, subdirectory);
		if (Files::pathExists(sub))
		{
			Files::moveSilent(sub, destination);
			Files::remove(d2, false);
		}
		else
		{
			// Error
		}
	}
	else
	{
		if (!Process::runNoOutput({ unzip, outputFile, "-d", destination }))
			return false;
	}

	Files::remove(outputFile);

	m_lastHash = hash;

	return true;
}

/*****************************************************************************/
bool ArchiveDependencyExtractor::needsUpdate(const ArchiveDependency& inDependency)
{
	const auto& destination = inDependency.destination();
	const auto& url = inDependency.url();

	if (!m_dependencyCache.contains(destination))
		return true;

	Json json = m_dependencyCache.get(destination);
	if (!json.is_object())
		json = Json::object();

	const auto hash = json["h"].is_string() ? json["h"].get<std::string>() : std::string();
	const auto cachedUrl = json["u"].is_string() ? json["u"].get<std::string>() : std::string();

	bool urlNeedsUpdate = cachedUrl != url;

	bool update = urlNeedsUpdate;
	bool isConfigure = m_centralState.inputs().route().isConfigure();
	if (!update && isConfigure)
	{
		Timer timer;
		displayCheckingForUpdates(destination);

		// Need some way to compare remote contents

		Diagnostic::printDone(timer.asString());
	}

	if (update)
	{
		if (!Files::removeRecursively(destination))
			return false;
	}

	return update;
}

/*****************************************************************************/
bool ArchiveDependencyExtractor::updateDependencyCache(const ArchiveDependency& inDependency)
{
	const auto& destination = inDependency.destination();
	const auto& url = inDependency.url();

	Json json;
	json["h"] = m_lastHash;
	json["u"] = url;

	if (m_dependencyCache.contains(destination))
	{
		m_dependencyCache.set(destination, std::move(json));
	}
	else
	{
		m_dependencyCache.emplace(destination, std::move(json));
	}

	// Do any cleanup here

	return true;
}

/*****************************************************************************/
void ArchiveDependencyExtractor::displayCheckingForUpdates(const std::string& inDestination)
{
	Diagnostic::infoEllipsis("Checking remote for updates: {}", inDestination);
}

/*****************************************************************************/
void ArchiveDependencyExtractor::displayFetchingMessageStart(const ArchiveDependency& inDependency)
{
	const auto& url = inDependency.url();

	auto path = url;

	Output::msgFetchingDependency(path);
}

}
