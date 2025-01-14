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
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"
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
	m_failure = false;

	auto onReplaceContents = [this](std::string& fileContents) {
		auto onReplace = [this](std::string match) {
			return this->getReplaceValue(std::move(match));
		};
		RegexPatterns::matchAndReplaceConfigureFileVariables(fileContents, onReplace);

		BuildState::VariableOptions options;
		options.checkHome = false;
		options.onFail = onReplace;
		m_state.replaceVariablesInString(fileContents, &m_project, options);
		replaceEmbeddable(fileContents, m_embedFileCache);

		if (fileContents.back() != '\n')
			fileContents += '\n';
	};

	auto& sources = m_state.cache.file().sources();
	const auto& configureFiles = m_project.configureFiles();

	constexpr char kConfigureFiles[] = "configureFiles";
	Dictionary<StringList> embeddedFileCache;
	auto intermediateDir = m_state.paths.intermediateDir(m_project);
	m_cacheFile = fmt::format("{}/{}_cache.json", intermediateDir, m_project.name());
	JsonFile jsonFile(m_cacheFile);
	if (!Files::pathExists(m_cacheFile) || jsonFile.load())
	{
		Json& jRoot = jsonFile.root;
		if (!jRoot.is_object())
			jRoot = Json::object();

		auto& jConfFiles = jRoot[kConfigureFiles];
		if (!jConfFiles.is_object())
			jConfFiles = Json::object();

		for (const auto& [name, jArray] : jConfFiles.items())
		{
			if (jArray.is_array())
			{
				for (auto& jValue : jArray)
				{
					auto value = json::get<std::string>(jValue);
					if (!value.empty() && sources.fileChangedOrDoesNotExist(value))
					{
						if (embeddedFileCache.find(name) == embeddedFileCache.end())
							embeddedFileCache.emplace(name, StringList{});
					}
				}
			}
		}
	}

	bool metadataChanged = m_state.cache.file().metadataChanged();

	std::string suffix(".in");
	if (!Files::pathExists(inOutputFolder))
		Files::makeDirectory(inOutputFolder);

	bool failure = false;
	for (const auto& configureFile : configureFiles)
	{
		if (!Files::pathExists(configureFile))
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
		bool pathExists = Files::pathExists(outPath);
		bool dependentChanged = embeddedFileCache.find(configureFile) != embeddedFileCache.end();
		if (configFileChanged || metadataChanged || !pathExists || dependentChanged)
		{
			if (configFileChanged || pathExists)
				Files::removeIfExists(outPath);

			if (!Files::copyRename(configureFile, outPath, true))
			{
				Diagnostic::error("There was a problem copying the file: {}", configureFile);
				result = false;
				continue;
			}

			if (!Files::readAndReplace(outPath, onReplaceContents))
			{
				Diagnostic::error("There was a problem parsing the file: {}", configureFile);
				result = false;
				continue;
			}

			embeddedFileCache[configureFile] = m_embedFileCache;
			m_embedFileCache.clear();

			if (m_failure)
			{
				Diagnostic::error("There was a problem parsing the file: {}", configureFile);
				failure |= m_failure;
				m_failure = false;
			}
		}
	}

	if (failure)
		return false;

	if (!embeddedFileCache.empty())
	{
		auto& jConfFiles = jsonFile.root[kConfigureFiles];
		for (auto&& [file, depends] : embeddedFileCache)
		{
			jConfFiles[file] = depends;
		}
		jsonFile.setDirty(true);
		jsonFile.save();
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

/*****************************************************************************/
void ConfigureFileParser::replaceEmbeddable(std::string& outContent, StringList& outCache)
{
	std::string kToken{ "$embed(\"" };
	size_t embedPos = outContent.find(kToken, 0);
	while (embedPos != std::string::npos)
	{
		auto beforeText = outContent.substr(0, embedPos);

		embedPos += kToken.size();
		auto closingBrace = outContent.find("\")", embedPos);
		bool valid = closingBrace != std::string::npos;
		if (valid)
		{
			auto afterText = outContent.substr(closingBrace + 2);

			auto file = outContent.substr(embedPos, closingBrace - embedPos);
			auto resolvedFile = Files::getCanonicalPath(file);
			if (Files::pathExists(resolvedFile))
			{
				List::addIfDoesNotExist(outCache, file);

				// Note: at the moment, we only generate as bytes
				//
				std::string text{ "'\\0'" };
				if (generateBytesForFile(text, resolvedFile))
				{
					outContent = beforeText + "{\n\t// clang-format off\n\t" + text + "\n\t// clang-format on\n}" + afterText;
					// String::replaceAll(outContent, fmt::format("{}{}\")", kToken, file), text);
				}
				else
				{
					Diagnostic::error("Error reading the embedded file: {}", file);
					m_failure = true;
				}
			}
			else
			{
				Diagnostic::error("Embedded file not found: {}", file);
				m_failure = true;
			}
		}
		embedPos = outContent.find(kToken, valid ? closingBrace + 1 : embedPos);
	}
}

/*****************************************************************************/
bool ConfigureFileParser::generateBytesForFile(std::string& outText, const std::string& inFile) const
{
	std::error_code ec;
	size_t fileSize = static_cast<size_t>(fs::file_size(inFile, ec));
	constexpr size_t maxSize = 1000000000; // cap the max embedded size at a GB for now
	if (fileSize > maxSize)
	{
		Diagnostic::error("File too large: {}", inFile);
		return false;
	}

	std::ifstream in(inFile, std::ios_base::binary);

	std::vector<int> bytes(fileSize);

	size_t idx = 0;
	while (in)
	{
		int value = in.get();
		if (in.fail())
			break;

		if (idx >= fileSize)
			return false;

		bytes[idx] = value;
		idx++;
	};

	if (bytes.empty())
		return false;

	outText.clear();

	constexpr size_t numColumns = 20;
	for (size_t i = 0; i < bytes.size(); ++i)
	{
		outText += fmt::format("{:#04x}, ", std::byte(bytes[i]));
		if (i % numColumns == numColumns - 1)
		{
			outText += '\n';
			outText += '\t';
		}
	}

	if (outText.back() == ' ')
		outText.pop_back();

	return true;
}
}
