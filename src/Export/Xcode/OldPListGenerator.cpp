/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/Xcode/OldPListGenerator.hpp"

#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
void OldPListGenerator::dumpToTerminal()
{
	auto output = json::dump(m_json, 2, ' ');
	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();
}

/*****************************************************************************/
// Note: Just used for .pbxproj files at the moment, so not sure if some of this is specific to that
//  or the general "old-style" plist format
//
std::string OldPListGenerator::getContents(const StringList& inSingleLineSections) const
{
	auto archiveVersion = m_json["archiveVersion"].get<i32>();
	auto objectVersion = m_json["objectVersion"].get<i32>();
	auto rootObject = m_json["rootObject"].get<std::string>();

	std::string sections;
	const auto& objects = m_json["objects"];
	for (auto& [key, value] : objects.items())
	{
		const auto& section = key;
		if (!value.is_object())
			continue;

		i32 indent = 2;
		if (String::equals(inSingleLineSections, section))
			indent = 0;

		sections += fmt::format("\n/* Begin {} section */\n", section);
		for (auto& [subkey, subvalue] : value.items())
		{
			if (!subvalue.is_object())
				continue;

			sections += std::string(2, '\t');
			sections += subkey;
			sections += " = ";
			sections += getNodeAsPListFormat(subvalue, indent);
			sections += ";\n";
		}
		sections += fmt::format("/* End {} section */\n", section);
	}

	sections.pop_back();

	auto contents = fmt::format(R"oldplist(// !$*UTF8*$!
{{
	archiveVersion = {archiveVersion};
	classes = {{
	}};
	objectVersion = {objectVersion};
	objects = {{
{sections}
	}};
	rootObject = {rootObject};
}})oldplist",
		FMT_ARG(archiveVersion),
		FMT_ARG(objectVersion),
		FMT_ARG(sections),
		FMT_ARG(rootObject));

	return contents;
}

/*****************************************************************************/
std::string OldPListGenerator::getNodeAsPListFormat(const Json& inJson, const size_t indent) const
{
	std::string ret;

	if (inJson.is_object())
	{
		ret += "{\n";

		if (inJson.contains("isa"))
		{
			const auto value = inJson["isa"].get<std::string>();
			ret += std::string(indent + 1, '\t');
			ret += fmt::format("isa = {};\n", value);
		}

		for (auto& [key, value] : inJson.items())
		{
			if (String::equals("isa", key))
				continue;

			ret += std::string(indent + 1, '\t');

			if (String::contains('[', key))
				ret += fmt::format("\"{}\"", key);
			else
				ret += key;

			ret += " = ";
			ret += getNodeAsPListFormat(value, indent + 1);
			ret += ";\n";
		}
		ret += std::string(indent, '\t');
		ret += '}';
	}
	else if (inJson.is_array())
	{
		ret += "(\n";
		for (auto& value : inJson)
		{
			ret += std::string(indent + 1, '\t');
			ret += getNodeAsPListString(value);
			ret += ",\n";
		}

		// removes last comma
		// ret.pop_back();
		// ret.pop_back();
		// ret += '\n';

		ret += std::string(indent, '\t');
		ret += ')';
	}
	else if (inJson.is_string())
	{
		ret += getNodeAsPListString(inJson);
	}
	else if (inJson.is_number_float())
	{
		ret += std::to_string(inJson.get<f32>());
	}
	else if (inJson.is_number_integer())
	{
		ret += std::to_string(inJson.get<i64>());
	}
	else if (inJson.is_number_unsigned())
	{
		ret += std::to_string(inJson.get<u64>());
	}

	if (indent == 0)
	{
		String::replaceAll(ret, '\n', ' ');
		String::replaceAll(ret, '\t', "");
	}

	return ret;
}

/*****************************************************************************/
std::string OldPListGenerator::getNodeAsPListString(const Json& inJson) const
{
	if (!inJson.is_string())
		return "\"\"";

	bool noQuotes = false;
	auto str = inJson.get<std::string>();
	if (str.size() > 24)
	{
		auto substring = str.substr(0, 23);
		if (substring.find_first_not_of("01234567890ABCDEF") == std::string::npos)
		{
			noQuotes = true;
		}
	}

	if (!str.empty() && !noQuotes)
	{
		noQuotes = String::startsWith('(', str);
	}

	if (!str.empty() && (noQuotes || str.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890_/") == std::string::npos))
	{
		return str;
	}
	else
	{
		String::replaceAll(str, '"', "\\\"");
		return fmt::format("\"{}\"", str);
	}
}
}
