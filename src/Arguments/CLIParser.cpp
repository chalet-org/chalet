/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/CLIParser.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CLIParser::RawArgumentList CLIParser::parse(const int argc, const char* argv[], const int inPositionalArgs)
{
	std::vector<std::pair<std::string, std::string>> ret;

	ret.emplace_back("@0", std::string(argv[0]));

	int j = 0;
	for (int i = 1; i < argc; ++i)
	{
		auto arg = std::string(argv[i]);
		if (arg[0] == '-')
		{
			if (String::contains('=', arg))
			{
				String::replaceAll(arg, "=true", "=1");
				String::replaceAll(arg, "=false", "=0");

				auto eq = arg.find('=');

				ret.emplace_back(arg.substr(0, eq), arg.substr(eq + 1));
				continue;
			}
			else
			{
				auto option = i + 1 < argc ? CLIParser::getOptionValue(argv + i, argv + i + 1, arg) : std::nullopt;
				if (option.has_value())
				{
					if ((*option)[0] != '-')
					{
						ret.emplace_back(std::move(arg), *option);
						++i;
					}
					else
					{
						ret.emplace_back(std::move(arg), "1");
					}
					continue;
				}
			}
		}
		else if (inPositionalArgs > 0)
		{
			ret.emplace_back(fmt::format("@{}", j + 1), arg);
			++j;

			if (j < inPositionalArgs)
				continue;

			bool notFirst = false;
			std::string rest;
			while (true)
			{
				++i;
				if (i < argc)
				{
					if (notFirst)
						rest += ' ';

					rest += argv[i];
					notFirst = true;
				}
				else
					break;
			}

			if (!rest.empty())
				ret.emplace_back("...", std::move(rest));
		}
	}

	return ret;
}

/*****************************************************************************/
std::optional<std::string> CLIParser::getOptionValue(const char** inBegin, const char** inEnd, const std::string& inOption)
{
	auto itr = std::find(inBegin, inEnd, inOption);
	if (itr != inEnd)
	{
		++itr;
		std::string ret(*itr);
		if (!ret.empty() /*&& ret.front() != '-'*/)
		{
			if (ret.front() == '"')
			{
				ret = ret.substr(1);

				if (ret.back() == '"')
					ret.pop_back();
			}

			if (ret.front() == '\'')
			{
				ret = ret.substr(1);

				if (ret.back() == '\'')
					ret.pop_back();
			}

			return ret;
		}
	}

	return std::nullopt;
}

/*****************************************************************************/
bool CLIParser::optionExists(const char** inBegin, const char** inEnd, const std::string& inOption)
{
	return std::find(inBegin, inEnd, inOption) != inEnd;
}

}
