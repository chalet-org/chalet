/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/ConfigureFileParser.hpp"

#include <regex>

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "State/TargetMetadata.hpp"
#include "State/WorkspaceEnvironment.hpp"
#include "Terminal/Commands.hpp"
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
bool ConfigureFileParser::run()
{
	// Timer timer;
	bool result = true;

	static std::regex reA(R"regex(@(\w+)@)regex");
	static std::regex reB(R"regex(\$\{(\w+)\})regex");

	auto onReplaceContents = [&](std::string& fileContents) {
		std::smatch sm;
		while (std::regex_search(fileContents, sm, reA))
		{
			const auto& replaceValue = getReplaceValue(sm[1].str());
			std::string prefix = sm.prefix();
			std::string suffix = sm.suffix();
			fileContents = fmt::format("{}{}{}", prefix, replaceValue, suffix);
		}

		while (std::regex_search(fileContents, sm, reB))
		{
			const auto& replaceValue = getReplaceValue(sm[1].str());
			std::string prefix = sm.prefix();
			std::string suffix = sm.suffix();
			fileContents = fmt::format("{}{}{}", prefix, replaceValue, suffix);
		}
	};

	auto& sources = m_state.cache.file().sources();

	bool metadataChanged = m_state.cache.file().metadataChanged();

	std::string suffix(".in");
	auto outFolder = m_state.paths.intermediateDir(m_project);
	for (const auto& configureFile : m_project.configureFiles())
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

		auto outPath = fmt::format("{}/{}", outFolder, outFile);

		bool configFileChanged = sources.fileChangedOrDoesNotExist(configureFile);
		if (configFileChanged || metadataChanged || !Commands::pathExists(outPath))
		{
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
	else if (String::startsWith("TARGET_", inKey))
	{
		inKey = inKey.substr(7);
		return getReplaceValueFromSubString(inKey, true);
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

	bool targetMetaData = isTarget && m_project.hasMetadata();
	const auto& workspaceMeta = m_state.workspace.metadata();
	if (String::equals("NAME", inKey))
	{
		if (targetMetaData && !m_project.metadata().name().empty())
			return m_project.metadata().name();
		else
			return workspaceMeta.name();
	}

	if (String::equals("DESCRIPTION", inKey))
	{
		if (targetMetaData && !m_project.metadata().description().empty())
			return m_project.metadata().description();
		else
			return workspaceMeta.description();
	}

	if (String::equals("HOMEPAGE_URL", inKey))
	{
		if (targetMetaData && !m_project.metadata().homepage().empty())
			return m_project.metadata().homepage();
		else
			return workspaceMeta.homepage();
	}

	if (String::equals("AUTHOR", inKey))
	{
		if (targetMetaData && !m_project.metadata().author().empty())
			return m_project.metadata().author();
		else
			return workspaceMeta.author();
	}

	if (String::equals("LICENSE", inKey))
	{
		if (targetMetaData && !m_project.metadata().license().empty())
			return m_project.metadata().license();
		else
			return workspaceMeta.license();
	}

	if (String::equals("README", inKey))
	{
		if (targetMetaData && !m_project.metadata().readme().empty())
			return m_project.metadata().readme();
		else
			return workspaceMeta.readme();
	}

	bool versionAvailable = targetMetaData && !m_project.metadata().versionString().empty();
	if (String::equals("VERSION", inKey))
	{
		if (versionAvailable)
			return m_project.metadata().versionString();
		else
			return workspaceMeta.versionString();
	}

	const auto& version = versionAvailable ? m_project.metadata().version() : workspaceMeta.version();

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
