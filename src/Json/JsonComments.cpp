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
bool JsonComments::parse(Json& outJson, const std::string& inFilename)
{
	if (!Commands::pathExists(inFilename))
	{
		outJson = Json();
		return false;
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
	CHALET_CATCH(const Json::out_of_range& err)
	{
		CHALET_EXCEPT_ERROR("{}", err.what());
		Diagnostic::error("Error parsing: {}", inFilename);
		outJson = Json();
		return false;
	}
	CHALET_CATCH(const Json::parse_error& err)
	{
		CHALET_EXCEPT_ERROR("{}", err.what());
		Diagnostic::error("Error parsing: {}", inFilename);
		outJson = Json();
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
	CHALET_CATCH(const Json::out_of_range& err)
	{
		CHALET_EXCEPT_ERROR("{}", err.what());
		return Json();
	}
	CHALET_CATCH(const Json::parse_error& err)
	{
		CHALET_EXCEPT_ERROR("{}", err.what());
		return Json();
	}
}
}