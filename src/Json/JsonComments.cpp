/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Json/JsonComments.hpp"

#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
Json JsonComments::parse(const std::string& inFilename)
{
	if (!Commands::pathExists(inFilename))
		return Json();

	std::string lines;
	std::ifstream fileStream(inFilename);

	try
	{
		nlohmann::detail::parser_callback_t<Json> cb = nullptr;
		bool allow_exceptions = true;
		bool ignore_comments = true;

		return Json::parse(fileStream, cb, allow_exceptions, ignore_comments);
	}
	catch (Json::parse_error& e)
	{
		Diagnostic::errorAbort(e.what(), "JsonComments::parseLiteral: Error parsing " + inFilename);
		return Json();
	}
}

/*****************************************************************************/
Json JsonComments::parseLiteral(const std::string& inJsonContent)
{
	try
	{
		nlohmann::detail::parser_callback_t<Json> cb = nullptr;
		bool allow_exceptions = true;
		bool ignore_comments = true;

		return Json::parse(inJsonContent, cb, allow_exceptions, ignore_comments);
	}
	catch (Json::parse_error& e)
	{
		Diagnostic::errorAbort(e.what(), "JsonComments::parseLiteral: Error parsing provided content");
		return Json();
	}
}
}