/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ConfigureFileParser.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
// #include "Utility/Timer.hpp"

namespace chalet
{
/*****************************************************************************/
ConfigureFileParser::ConfigureFileParser(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
}

/*****************************************************************************/
bool ConfigureFileParser::run(const std::string& inOutputFolder)
{
	if (inOutputFolder.empty())
	{
		Diagnostic::error("bad path sent to ConfigureFileParser");
		return false;
	}

	// Timer timer;
	bool result = true;

	auto onReplaceContents = [this](std::string& fileContents) {
		auto onReplace = [this](auto match) {
			return this->getReplaceValue(std::move(match));
		};
		RegexPatterns::matchAndReplaceConfigureFileVariables(fileContents, onReplace);
		m_state.replaceVariablesInString(fileContents, &m_project, false, onReplace);

		if (fileContents.back() != '\n')
			fileContents += '\n';
	};

	auto& sources = m_state.cache.file().sources();
	const auto& configureFiles = m_project.configureFiles();

	bool metadataChanged = m_state.cache.file().metadataChanged();

	std::string suffix(".in");
	if (!Commands::pathExists(inOutputFolder))
		Commands::makeDirectory(inOutputFolder);

	for (const auto& configureFile : configureFiles)
	{
		if (!Commands::pathExists(configureFile))
		{
			Diagnostic::error("Configure file not found: {}", configureFile);
			result = false;
			continue;
		}

		if (!String::endsWith(suffix, configureFile))
		{
			Diagnostic::error("Configure file must end with '.in': {}", configureFile);
			result = false;
			continue;
		}

		auto outFile = String::getPathFilename(configureFile);
		outFile = outFile.substr(0, outFile.size() - 3);

		auto outPath = fmt::format("{}/{}", inOutputFolder, outFile);

		bool configFileChanged = sources.fileChangedOrDoesNotExist(configureFile);
		bool pathExists = Commands::pathExists(outPath);
		if (configFileChanged || metadataChanged || !pathExists)
		{
			if (pathExists)
				Commands::remove(outPath);

			if (!Commands::copyRename(configureFile, outPath, true))
			{
				Diagnostic::error("There was a problem copying the file: {}", configureFile);
				result = false;
				continue;
			}

			if (!Commands::readFileAndReplace(outPath, onReplaceContents))
			{
				Diagnostic::error("There was a problem parsing the file: {}", configureFile);
				result = false;
				continue;
			}
		}
	}

	// LOG("completed in:", timer.asString());

	return result;
}

/*****************************************************************************/
std::string ConfigureFileParser::getReplaceValue(std::string inKey) const
{
	if (String::startsWith("WORKSPACE_", inKey))
	{
		inKey = inKey.substr(10);
		return getReplaceValueFromSubString(inKey);
	}
	else if (String::startsWith("CMAKE_PROJECT_", inKey)) // CMake compatibility
	{
		inKey = inKey.substr(14);
		return getReplaceValueFromSubString(inKey);
	}
	else if (String::startsWith("PROJECT_", inKey)) // CMake compatibility
	{
		inKey = inKey.substr(8);
		return getReplaceValueFromSubString(inKey, true);
	}

	return std::string();
}

/*****************************************************************************/
std::string ConfigureFileParser::getReplaceValueFromSubString(const std::string& inKey, const bool isTarget) const
{
	// LOG(inKey);
	UNUSED(isTarget);

	const bool targetHasMetadata = isTarget && m_project.hasMetadata();
	const auto& metadata = targetHasMetadata ? m_project.metadata() : m_state.workspace.metadata();

	if (String::equals("NAME", inKey))
		return metadata.name();

	if (String::equals("DESCRIPTION", inKey))
		return metadata.description();

	if (String::equals("HOMEPAGE_URL", inKey))
		return metadata.homepage();

	if (String::equals("AUTHOR", inKey))
		return metadata.author();

	if (String::equals("LICENSE", inKey))
		return metadata.license();

	if (String::equals("README", inKey))
		return metadata.readme();

	if (String::equals("VERSION", inKey))
		return metadata.versionString();

	const auto& version = metadata.version();

	if (String::equals("VERSION_MAJOR", inKey) && version.hasMajor())
		return std::to_string(version.major());

	if (String::equals("VERSION_MINOR", inKey) && version.hasMinor())
		return std::to_string(version.minor());

	if (String::equals("VERSION_PATCH", inKey) && version.hasPatch())
		return std::to_string(version.patch());

	if (String::equals("VERSION_TWEAK", inKey) && version.hasTweak())
		return std::to_string(version.tweak());

	return std::string();
}
}
