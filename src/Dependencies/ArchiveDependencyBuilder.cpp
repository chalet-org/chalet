/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Dependencies/ArchiveDependencyBuilder.hpp"

#include "Cache/ExternalDependencyCache.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
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
ArchiveDependencyBuilder::ArchiveDependencyBuilder(CentralState& inCentralState, const ArchiveDependency& inDependency) :
	m_centralState(inCentralState),
	m_archiveDependency(inDependency),
	m_dependencyCache(m_centralState.cache.file().externalDependencies())
{
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::run(StringList& outChanged)
{
	const auto& destination = m_archiveDependency.destination();

	bool destinationExists = Files::pathExists(destination);
	if (!localPathShouldUpdate(destinationExists))
		return true;

	destinationExists = Files::pathExists(destination);
	if (fetchDependency(destinationExists))
	{
		if (!updateDependencyCache())
		{
			Diagnostic::error("Error fetching archive dependency: {}", m_archiveDependency.name());
			return false;
		}

		outChanged.emplace_back(Files::getCanonicalPath(destination));
		return true;
	}
	else
	{
		if (Files::pathExists(destination))
			Files::removeRecursively(destination);
	}

	Diagnostic::error("Error fetching archive dependency: {}", m_archiveDependency.name());
	return false;
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::localPathShouldUpdate(const bool inDestinationExists)
{
	const auto& destination = m_archiveDependency.destination();
	if (!m_dependencyCache.contains(destination))
	{
		if (inDestinationExists)
			return Files::removeRecursively(destination);

		return true;
	}

	if (!inDestinationExists)
		return true;

	return needsUpdate();
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::fetchDependency(const bool inDestinationExists)
{
	if (inDestinationExists && m_dependencyCache.contains(m_archiveDependency.destination()))
		return true;

	displayFetchingMessageStart();

	if (!validateTools())
		return false;

	const auto destination = getTempDestination();

	const auto& url = m_archiveDependency.url();
	const auto& subdirectory = m_archiveDependency.subdirectory();

	Files::makeDirectory(destination);

	const auto& curl = m_centralState.tools.curl();
	auto outputFile = getOutputFile();
	if (!Process::run({ curl, "-s", "-L", "-o", outputFile, url }))
		return false;

	// zip
	m_lastHash = getArchiveHash(outputFile);

	auto format = m_archiveDependency.format();
	if (format == ArchiveFormat::Zip)
	{
		if (!extractZipFile(outputFile, destination))
			return false;
	}
	else if (format == ArchiveFormat::Tar)
	{
		if (!extractTarFile(outputFile, destination))
			return false;
	}
	else
	{
		return false;
	}

	if (!subdirectory.empty())
	{
		auto sub = fmt::format("{}/{}", destination, subdirectory);
		if (Files::pathExists(sub))
		{
			Files::moveSilent(sub, m_archiveDependency.destination());
			Files::removeRecursively(destination);
		}
		else
		{
			Diagnostic::error("Archive expected a subdirectory of '{}', but it was not found.", subdirectory);
			return false;
		}
	}

	Files::remove(outputFile);

	return true;
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::needsUpdate()
{
	const auto& destination = m_archiveDependency.destination();
	const auto& url = m_archiveDependency.url();
	const auto& subdirectory = m_archiveDependency.subdirectory();

	if (!m_dependencyCache.contains(destination))
		return true;

	Json jRoot = m_dependencyCache.get(destination);
	if (!jRoot.is_object())
		jRoot = Json::object();

	const auto hash = json::get<std::string>(jRoot, "h");
	const auto cachedUrl = json::get<std::string>(jRoot, "u");
	const auto cachedSubdirectory = json::get<std::string>(jRoot, "s");

	const bool urlNeedsUpdate = cachedUrl != url;
	const bool subdirectoryNeedsUpdate = cachedSubdirectory != subdirectory;

	const bool isConfigure = m_centralState.inputs().route().isConfigure();
	bool update = urlNeedsUpdate || subdirectoryNeedsUpdate;
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
bool ArchiveDependencyBuilder::updateDependencyCache()
{
	const auto& destination = m_archiveDependency.destination();
	const auto& url = m_archiveDependency.url();
	const auto& subdirectory = m_archiveDependency.subdirectory();

	Json data;
	data["h"] = m_lastHash;
	data["u"] = url;
	data["s"] = subdirectory;

	if (m_dependencyCache.contains(destination))
	{
		m_dependencyCache.set(destination, std::move(data));
	}
	else
	{
		m_dependencyCache.emplace(destination, std::move(data));
	}

	// Do any cleanup here

	return true;
}

/*****************************************************************************/
void ArchiveDependencyBuilder::displayCheckingForUpdates(const std::string& inDestination)
{
	Diagnostic::infoEllipsis("Checking remote for updates: {}", inDestination);
}

/*****************************************************************************/
void ArchiveDependencyBuilder::displayFetchingMessageStart()
{
	const auto& url = m_archiveDependency.url();

	auto path = url;

	Output::msgFetchingDependency(path);
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::validateTools() const
{
	const auto& curl = m_centralState.tools.curl();
	if (curl.empty())
	{
		Diagnostic::error("archive dependency requires curl: {}", m_archiveDependency.name());
		return false;
	}

	auto format = m_archiveDependency.format();
	if (format == ArchiveFormat::Zip)
	{
#if !defined(CHALET_WIN32)
		const auto& unzip = m_centralState.tools.unzip();
		if (unzip.empty())
		{
			Diagnostic::error("archive dependency requires unzip: {}", m_archiveDependency.name());
			return false;
		}
#endif
	}
	else if (format == ArchiveFormat::Tar)
	{
		const auto& tar = m_centralState.tools.tar();
		if (tar.empty())
		{
			Diagnostic::error("archive dependency requires tar: {}", m_archiveDependency.name());
			return false;
		}
	}

#if defined(CHALET_WIN32)
	const auto& powershell = m_centralState.tools.powershell();
	if (powershell.empty())
	{
		Diagnostic::error("archive dependency requires powershell: {}", m_archiveDependency.name());
		return false;
	}
#else
	const auto& openssl = m_centralState.tools.openssl();
	if (openssl.empty())
	{
		Diagnostic::error("archive dependency requires openssl: {}", m_archiveDependency.name());
		return false;
	}
#endif

	return true;
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::extractZipFile(const std::string& inFilename, const std::string& inDestination) const
{
#if defined(CHALET_WIN32)
	StringList pwshCmd{
		"Expand-Archive",
		"-Force",
		"-LiteralPath",
		inFilename,
		"-DestinationPath",
		inDestination
	};

	// Note: We need to hide the weird MS progress dialog (Write-Progress),
	//   which is done with $ProgressPreference

	const auto& powershell = m_centralState.tools.powershell();

	StringList cmd;
	cmd.emplace_back(powershell);
	cmd.emplace_back("-Command");
	cmd.emplace_back("$ProgressPreference = \"SilentlyContinue\";");
	cmd.emplace_back(fmt::format("{};", String::join(pwshCmd)));
	cmd.emplace_back("$ProgressPreference = \"Continue\";");

	if (!Process::runNoOutput(cmd))
		return false;
#else
	const auto& unzip = m_centralState.tools.unzip();
	if (!Process::runNoOutput({ unzip, inFilename, "-d", inDestination }))
		return false;
#endif

	return true;
}

/*****************************************************************************/
bool ArchiveDependencyBuilder::extractTarFile(const std::string& inFilename, const std::string& inDestination) const
{
	StringList cmd;

	// #if defined(CHALET_WIN32)
	// 	const auto& powershell = m_centralState.tools.powershell();
	// 	cmd.emplace_back(powershell);
	// 	cmd.emplace_back("tar");
	// #else
	// 	const auto& tar = m_centralState.tools.tar();
	// 	cmd.emplace_back(tar);
	// #endif
	const auto& tar = m_centralState.tools.tar();
	cmd.emplace_back(tar);

	cmd.emplace_back("-x");
	cmd.emplace_back("-v");
	cmd.emplace_back("-z");
	cmd.emplace_back("-f");
	cmd.emplace_back(inFilename);
	cmd.emplace_back("-C");
	cmd.emplace_back(inDestination);

	if (!Process::runNoOutput(cmd))
		return false;

	return true;
}

/*****************************************************************************/
std::string ArchiveDependencyBuilder::getTempDestination() const noexcept
{
	const auto& destination = m_archiveDependency.destination();

	if (!m_archiveDependency.subdirectory().empty())
		return fmt::format("{}/tmp", destination);
	else
		return destination;
}

/*****************************************************************************/
std::string ArchiveDependencyBuilder::getOutputFile() const noexcept
{
	const auto& url = m_archiveDependency.url();
	const auto& destination = m_archiveDependency.destination();

	auto filename = String::getPathFilename(url);
	return fmt::format("{}/{}", destination, filename);
}

/*****************************************************************************/
std::string ArchiveDependencyBuilder::getArchiveHash(const std::string& inFilename) const
{
#if defined(CHALET_WIN32)
	StringList pwshCmd{
		"Get-FileHash",
		inFilename,
		"| Select-Object Hash | Format-List"
	};

	const auto& powershell = m_centralState.tools.powershell();
	auto shaOutput = Process::runOutput({ powershell, String::join(pwshCmd) });
#else
	const auto& openssl = m_centralState.tools.openssl();
	auto shaOutput = Process::runOutput({ openssl, "sha1", inFilename });
#endif
	return Hash::string(shaOutput);
}
}
