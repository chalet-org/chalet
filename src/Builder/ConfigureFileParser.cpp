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

	if (String::equals("NAME", inKey))
		return m_project.name();

	const auto& metadata = m_state.workspace.metadata();

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
