/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/BaseArgumentParser.hpp"

#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool BaseArgumentParser::parse(const int argc, const char* argv[], const int inPositionalArgs)
{
	m_rawArguments.clear();

	m_rawArguments.emplace("@0", std::string(argv[0]));

	auto truthyArguments = getTruthyArguments();

	int j = 0;
	for (int i = 1; i < argc; ++i)
	{
		auto arg = std::string(argv[i]);
		if (arg[0] == '-')
		{
			if (List::contains(truthyArguments, arg))
			{
				if (String::startsWith("--no-", arg))
					m_rawArguments.emplace(std::move(arg), "0");
				else
					m_rawArguments.emplace(std::move(arg), "1");
				continue;
			}
			else if (String::contains('=', arg))
			{
				String::replaceAll(arg, "=true", "=1");
				String::replaceAll(arg, "=false", "=0");

				auto eq = arg.find('=');

				m_rawArguments.emplace(arg.substr(0, eq), arg.substr(eq + 1));
				continue;
			}
			else
			{
				auto option = i + 1 < argc ? BaseArgumentParser::getOptionValue(argv + i, argv + i + 1, arg) : std::nullopt;
				if (option.has_value())
				{
					if ((*option)[0] != '-')
					{
						m_rawArguments.emplace(std::move(arg), *option);
						++i;
						continue;
					}
					else
					{
						m_rawArguments.emplace(std::move(arg), "1");
					}
				}
				else
				{
					m_rawArguments.emplace(std::move(arg), std::string());
				}
			}
		}
		else if (inPositionalArgs > 0)
		{
			if (arg.front() == '\'' && arg.back() == '\'')
			{
				arg.pop_back();
				arg = arg.substr(1);
			}

			m_rawArguments.emplace(fmt::format("@{}", j + 1), arg);
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
				m_rawArguments.emplace("...", std::move(rest));
		}
	}

	return true;
}

/*****************************************************************************/
bool BaseArgumentParser::containsOption(const std::string& inOption)
{
	return m_rawArguments.find(inOption) != m_rawArguments.end();
}

bool BaseArgumentParser::containsOption(const std::string& inShort, const std::string& inLong)
{
	return containsOption(inShort) || containsOption(inLong);
}

/*****************************************************************************/
std::optional<std::string> BaseArgumentParser::getOptionValue(const char** inBegin, const char** inEnd, const std::string& inOption)
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
bool BaseArgumentParser::optionExists(const char** inBegin, const char** inEnd, const std::string& inOption)
{
	return std::find(inBegin, inEnd, inOption) != inEnd;
}

}
