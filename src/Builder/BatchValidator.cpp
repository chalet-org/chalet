/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Builder/BatchValidator.hpp"

#include "Cache/SourceCache.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "State/BuildState.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"
#include "Yaml/YamlFile.hpp"
#include "Json/JsonComments.hpp"
#include "Json/JsonValidationError.hpp"
#include "Json/JsonValidator.hpp"

namespace chalet
{
/*****************************************************************************/
BatchValidator::BatchValidator(const BuildState* inState, const std::string& inSchemaFile) :
	m_state(inState),
	m_schemaFile(inSchemaFile)
{
}

/*****************************************************************************/
bool BatchValidator::validate(const StringList& inFiles, const bool inCache)
{
	CHALET_TRY
	{
		bool result = true;

		SourceCache* sourceCache = nullptr;
		if (m_state != nullptr && inCache)
		{
			sourceCache = &m_state->cache.file().sources();
		}

		auto cwd = Files::getWorkingDirectory();
		Path::toUnix(cwd);
		cwd += '/';
		m_schemaFile = Files::getCanonicalPath(m_schemaFile);
		String::replaceAll(m_schemaFile, cwd, "");

		bool schemaChanged = !inCache || (sourceCache && sourceCache->fileChangedOrDoesNotExistWithCache(m_schemaFile));

		// After files have been checked for changes
		StringList files;
		if (schemaChanged)
		{
			files = inFiles;
		}
		else if (sourceCache != nullptr)
		{
			for (auto& file : inFiles)
			{
				if (sourceCache->fileChangedOrDoesNotExistWithCache(file))
					files.emplace_back(file);
			}
		}

		Json schema;
		JsonValidator validator;
		if (!files.empty())
		{
			bool res = this->parse(schema, m_schemaFile, false);
			if (sourceCache)
				sourceCache->addOrRemoveFileCache(m_schemaFile, res);
			if (!res)
				return false;

			auto schemaUrl = json::get<std::string>(schema, "$schema");
			if (!schemaUrl.empty())
			{
				auto draft07 = "http://json-schema.org/draft-07/schema";
				if (!String::equals(draft07, schemaUrl))
				{
					showErrorMessage(fmt::format("Validation targets require '$schema' defined with the value '{}'", draft07));
					return false;
				}
			}
			else
			{
				showErrorMessage("validation targets require a '$schema' key, but none was found.");
				return false;
			}

			validator.setSchema(schema);
		}

		if (!files.empty())
		{
			Output::printCommand(fmt::format("   Schema: {}", m_schemaFile));
		}

		for (auto& file : files)
		{
			Path::toUnix(file);
			file = Files::getCanonicalPath(file);
			String::replaceAll(file, cwd, "");

			Diagnostic::subInfoEllipsis("{}", file);

			Json jsonFile;
			bool res = this->parse(jsonFile, file, true);
			if (sourceCache)
				sourceCache->addOrRemoveFileCache(file, res);
			if (!res)
			{
				result = false;
				continue;
			}

			JsonValidationErrors errors;
			bool fileValid = validator.validate(jsonFile, file, errors);
			if (sourceCache)
				sourceCache->addOrRemoveFileCache(file, fileValid);
			if (!fileValid)
			{
				Diagnostic::error("File: {}", file);
				if (!validator.printErrors(errors))
					result = false;
			}
			else
			{
				Diagnostic::printValid(true);
			}
		}

		if (result)
		{
			if (files.empty())
			{
				// Output::printCommand("   No files changed. Skipping.");
			}
			else
			{
				Output::lineBreak();

				auto filesPrint = files.size() > 1 ? "files" : "file";
				Output::printCommand(fmt::format("   Success! {} {} passed validation.", files.size(), filesPrint));
			}
		}
		else
		{
			Diagnostic::printValid(false);
			Diagnostic::printErrors(true);
		}

		return result;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_THROW(std::runtime_error(err.what()));
	}
}

/*****************************************************************************/
bool BatchValidator::parse(Json& outJson, const std::string& inFilename, const bool inPrintValid) const
{
	if (!Files::pathExists(inFilename))
	{
		outJson = Json();
		return true;
	}

	if (String::endsWith(".yaml", inFilename))
	{
		return YamlFile::parse(outJson, inFilename);
	}

	std::string lines;
	auto fileStream = Files::ifstream(inFilename);

	CHALET_TRY
	{
		nlohmann::detail::parser_callback_t<Json> cb = nullptr;
		bool allow_exceptions = true;
		bool ignore_comments = true;

		outJson = Json::parse(fileStream, cb, allow_exceptions, ignore_comments);
		return true;
	}
	CHALET_CATCH(const std::exception& err)
	{
		auto error = err.what();
		{
			auto jsonFile = Files::ifstream(inFilename);
			std::string output;
			bool printResult = JsonComments::printLinesWithError(jsonFile, error, output);

			if (inPrintValid)
				Diagnostic::printValid(false);
			else
				showErrorMessage(inFilename);

			showErrorMessage(output);

			if (!printResult)
				showErrorMessage(error);

			if (inPrintValid)
				Output::lineBreak();
		}
		outJson = Json();
		return false;
	}
}

/*****************************************************************************/
void BatchValidator::showErrorMessage(const std::string& inMessage) const
{
	const auto& color = Output::getAnsiStyle(Output::theme().error);
	const auto& reset = Output::getAnsiStyle(Output::theme().reset);

	std::cout << fmt::format("{}ERROR: {}{}\n", color, reset, inMessage);
}
}
