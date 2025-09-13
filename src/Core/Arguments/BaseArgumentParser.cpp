/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Arguments/BaseArgumentParser.hpp"

#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
StringList BaseArgumentParser::getArgumentList(const i32 argc, const char* argv[])
{
	StringList ret;

	for (i32 i = 0; i < argc; ++i)
	{
		ret.emplace_back(std::string(argv[i]));
	}

	return ret;
}

/*****************************************************************************/
bool BaseArgumentParser::parseArgument(size_t& index, std::string& arg, const std::string* nextArgPtr)
{
	if (arg.empty())
		return true; // ignore it

	if (arg.front() != '-')
		return false;

	// We need this list to make the distinction between key/value pairs and truthy/positional pairs
	//
	if (List::contains(m_truthyArguments, arg))
	{
		if (String::startsWith("--no-", arg))
			m_rawArguments.emplace(arg, "0");
		else
			m_rawArguments.emplace(arg, "1");
	}
	else if (String::contains('=', arg))
	{
		String::replaceAll(arg, "=true", "=1");
		String::replaceAll(arg, "=false", "=0");

		auto eq = arg.find('=');

		m_rawArguments.emplace(arg.substr(0, eq), arg.substr(eq + 1));
	}
	else
	{
		if (nextArgPtr != nullptr)
		{
			auto& nextArg = *nextArgPtr;
			if ((!nextArg.empty() && nextArg.front() != '-') || nextArg.empty())
			{
				m_rawArguments.emplace(arg, nextArg);
				index++;
			}
		}
		else
		{
			m_rawArguments.emplace(arg, std::string());
			index++;
		}

		// Previous logic
		// if (nextArgPtr != nullptr)
		// {
		// 	auto& nextArg = *nextArgPtr;
		// 	if (!nextArg.empty() && nextArg.front() != '-')
		// 	{
		// 		m_rawArguments.emplace(arg, nextArg);
		// 		index++;
		// 	}
		// 	else
		// 	{
		// 		m_rawArguments.emplace(arg, "1");
		// 	}
		// }
		// else
		// {
		// 	m_rawArguments.emplace(arg, "1");
		// }
	}

	return true;
}

/*****************************************************************************/
void BaseArgumentParser::parseArgumentValue(std::string& arg)
{
	if (arg.empty() || arg[0] == '-')
		return;

	if (arg.front() == '"')
	{
		arg = arg.substr(1);

		if (arg.back() == '"')
			arg.pop_back();
	}

	if (arg.front() == '\'')
	{
		arg = arg.substr(1);

		if (arg.back() == '\'')
			arg.pop_back();
	}
}

/*****************************************************************************/
bool BaseArgumentParser::parse(StringList&& args, const u32 inPositionalArgs)
{
	if (args.empty())
		return false;

	m_rawArguments.clear();

	m_rawArguments.emplace("@0", args.front());

	m_truthyArguments = getTruthyArguments();

	u32 j = 0;
	for (size_t i = 1; i < args.size(); ++i)
	{
		auto& arg = args[i];
		std::string* nextArg = i + 1 < args.size() ? &args[i + 1] : nullptr;
		if (nextArg != nullptr)
			parseArgumentValue(*nextArg);

		if (parseArgument(i, arg, nextArg))
		{
			continue;
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

			++i;
			while (i < args.size())
			{
				m_remainingArguments.emplace_back(std::move(args[i]));
				++i;
			}
		}
	}

	if (!m_remainingArguments.empty())
		m_rawArguments.emplace("...", "1");

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

}
