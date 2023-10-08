/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonComments.hpp"

#include "Terminal/Commands.hpp"
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
		error[0] = static_cast<char>(::toupper(static_cast<uchar>(error[0])));
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

	int lineNo = 0;
	int columnNo = 0;
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

	auto colorGray = Output::getAnsiStyle(Output::theme().flair);
	auto colorError = Output::getAnsiStyle(Output::theme().error);
	auto colorReset = Output::getAnsiStyle(Output::theme().reset);

	int i = 0;
	for (std::string line; std::getline(inContents, line); ++i)
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
				std::size_t columnIndex = static_cast<std::size_t>(columnNo - 1);
				for (std::size_t j = 0; j < line.size(); ++j)
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
	if (!Commands::pathExists(inFilename))
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
		if (inError)
		{
			auto error = err.what();
			{
				auto msg = "There was a problem reading the json file";
				std::ifstream jsonFile(inFilename);
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