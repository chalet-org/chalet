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
#include "Utility/String.hpp"
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
		bool schemaChanged = !inCache || (sourceCache && sourceCache->fileChangedOrDoesNotExist(m_schemaFile));

		// After files have been checked for changes
		StringList files;
		if (schemaChanged)
		{
			files = inFiles;
		}
		else
		{
			for (auto& file : inFiles)
			{
				if (sourceCache && sourceCache->fileChangedOrDoesNotExist(file))
					files.emplace_back(file);
			}
		}

		Json schema;
		JsonValidator validator;
		if (!files.empty())
		{
			if (!this->parse(schema, m_schemaFile, false))
			{
				if (sourceCache)
					sourceCache->markForLater(m_schemaFile);
				return false;
			}

			if (schema.contains("$schema") && schema.at("$schema").is_string())
			{
				auto schemaUrl = schema.at("$schema").get<std::string>();
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

			validator.setSchema(std::move(schema));
		}

		if (!files.empty())
		{
			Output::printCommand(fmt::format("   Schema: {}", m_schemaFile));
		}

		for (auto& file : files)
		{
			Diagnostic::subInfoEllipsis("{}", file);

			Json jsonFile;
			if (!this->parse(jsonFile, file, true))
			{
				if (sourceCache)
					sourceCache->markForLater(file);

				result = false;
				continue;
			}

			JsonValidationErrors errors;
			bool fileValid = validator.validate(jsonFile, file, errors);
			if (!fileValid)
			{
				if (sourceCache)
					sourceCache->markForLater(file);

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
				Output::printCommand("   No files changed. Skipping.");
			}
			else
			{
				Output::lineBreak();

				auto filesPrint = files.size() > 1 ? "files" : "file";
				Output::printCommand(fmt::format("   Success! {} {} passed validation.", files.size(), filesPrint));
			}
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

	std::string lines;
	std::ifstream fileStream(inFilename);

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
			std::ifstream jsonFile(inFilename);
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
	const auto color = Output::getAnsiStyle(Output::theme().error);
	const auto reset = Output::getAnsiStyle(Output::theme().reset);

	std::cout << fmt::format("{}ERROR: {}{}\n", color, reset, inMessage);
}
}
