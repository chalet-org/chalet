/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonComments.hpp"

#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool JsonComments::printLinesWithError(std::basic_istream<char>& inContents, const char* inError, std::string& outOutput)
{
	std::string error(inError);
	auto lastSquareBrace = error.find(']');
	if (lastSquareBrace != std::string::npos)
	{
		error = error.substr(lastSquareBrace + 2);
		String::capitalize(error);
	}

	auto start = error.find("at line ");
	if (start == std::string::npos)
		return false;

	start += 8;
	auto end = error.find(",", start + 1);
	if (end == std::string::npos)
		return false;

	auto lineRaw = error.substr(start, end - start);

	start = error.find("column ", end);
	if (start == std::string::npos)
		return false;

	start += 7;
	end = error.find(":", start + 1);
	if (end == std::string::npos)
		return false;

	auto columnRaw = error.substr(start, end - start);

	if (lineRaw.empty() || columnRaw.empty())
		return false;

	i32 lineNo = 0;
	i32 columnNo = 0;
	CHALET_TRY
	{
		lineNo = std::stoi(lineRaw);
		columnNo = std::stoi(columnRaw);
	}
	CHALET_CATCH(...)
	{
		return false;
	}

	outOutput = fmt::format("{}\n", error);

	const auto& colorGray = Output::getAnsiStyle(Output::theme().flair);
	const auto& colorError = Output::getAnsiStyle(Output::theme().error);
	const auto& colorReset = Output::getAnsiStyle(Output::theme().reset);

	i32 i = 0;
	auto lineEnd = inContents.widen('\n');
	for (std::string line; std::getline(inContents, line, lineEnd); ++i)
	{
		if (i >= lineNo - 4 && i <= lineNo + 2)
		{
			if (i > 0)
				outOutput += '\n';

			bool current = i == lineNo - 1;
			auto& color = current ? colorError : colorGray;
			std::string outLine;
			if (current)
			{
				size_t columnIndex = static_cast<size_t>(columnNo - 1);
				for (size_t j = 0; j < line.size(); ++j)
				{
					if (j == columnIndex)
						outLine += fmt::format("{}{}{}", colorError, line[j], colorReset);
					else
						outLine += line[j];
				}
			}
			else
			{
				outLine = std::move(line);
			}
			outOutput += fmt::format("{}{} | {}{}", color, i + 1, colorReset, outLine);
		}
	}

	return true;
}

/*****************************************************************************/
bool JsonComments::parse(Json& outJson, const std::string& inFilename, const bool inError)
{
	if (!Files::pathExists(inFilename))
	{
		outJson = Json();
		return true;
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
		if (inError)
		{
			auto error = err.what();
			{
				auto msg = "There was a problem reading the json file";
				auto jsonFile = Files::ifstream(inFilename);
				std::string output;
				auto firstMessage = fmt::format("{}: {}", msg, inFilename);
				bool printResult = printLinesWithError(jsonFile, error, output);

				Diagnostic::error("{}", output);
				Diagnostic::error("{}", firstMessage);

				if (!printResult)
				{
					Diagnostic::error("{}", error);
					Diagnostic::error("{}: {}", msg, inFilename);
				}
			}
			outJson = Json();
		}
		return false;
	}
}

/*****************************************************************************/
Json JsonComments::parseLiteral(const std::string& inJsonContent)
{
	CHALET_TRY
	{
		nlohmann::detail::parser_callback_t<Json> cb = nullptr;
		bool allow_exceptions = true;
		bool ignore_comments = true;

		return Json::parse(inJsonContent, cb, allow_exceptions, ignore_comments);
	}
	CHALET_CATCH(const std::exception& err)
	{
		auto error = err.what();
		{
			auto msg = "There was a problem reading the json";
			std::stringstream stream;
			stream << inJsonContent;
			std::string output;
			auto firstMessage = fmt::format("{}", msg);
			bool printResult = printLinesWithError(stream, error, output);

			Diagnostic::error("{}", output);
			Diagnostic::error("{}", firstMessage);

			if (!printResult)
			{
				Diagnostic::error("{}", error);
				Diagnostic::error("{}", msg);
			}
		}
		return Json();
	}
}
}