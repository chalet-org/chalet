/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Yaml/YamlFile.hpp"

#include "System/Files.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
bool YamlFile::parse(Json& outJson, const std::string& inFilename, const bool inError)
{
	UNUSED(inError);

	YamlFile file(inFilename);
	return file.parseAsJson(outJson);
}

/*****************************************************************************/
bool YamlFile::saveToFile(const Json& inJson, const std::string& inFilename)
{
	YamlFile file(inFilename);
	return file.save(inJson);
}

/*****************************************************************************/
std::string YamlFile::asString(const Json& inJson)
{
	std::string filename;
	YamlFile file(filename);
	auto res = file.getNodeAsString(std::string(), inJson);

	if (!res.empty() && res.back() == '\n')
		res.pop_back();

	return res;
}

/*****************************************************************************/
YamlFile::YamlFile(const std::string& inFilename) :
	m_filename(inFilename)
{
}

/*****************************************************************************/
// Note: doesn't support arrays of objects yet (not used anyway)
//
bool YamlFile::parseAsJson(Json& outJson)
{
	if (!Files::pathExists(m_filename))
		return false;

	outJson = Json::object();

	std::vector<Json*> nodes;

	nodes.push_back(&outJson);

	size_t indent = 0;
	size_t lastIndent = 0;

	std::ifstream input(m_filename);
	for (std::string line; std::getline(input, line);)
	{
		if (line.empty())
			continue;

		// LOG(line);

		indent = 0;

		while (String::startsWith("  ", line))
		{
			line = line.substr(2);
			indent++;
		}

		bool arrayIsh = String::startsWith("- ", line);
		if (arrayIsh)
			indent = lastIndent + 1;
		else
			lastIndent = indent;

		if (arrayIsh)
			line = line.substr(2);

		while (!line.empty() && line.back() == ' ')
			line.pop_back();

		if (line.empty())
			continue;

		// ignore comments
		if (line.front() == '#')
			continue;

		while (nodes.size() > indent + 1)
			nodes.pop_back();

		chalet_assert(!nodes.empty(), "");

		// Start of object or array - just the key
		if (line.back() == ':')
		{
			line.pop_back();

			if (nodes.back()->is_array())
				return false;

			// We got here because the previous node was an object
			if (nodes.back()->is_null())
				(*nodes.back()) = Json::object();

			auto& node = *nodes.back();
			node[line] = Json();
			nodes.push_back(&node.at(line));
			continue;
		}

		auto& node = *nodes.back();

		// Objects
		auto firstKeyValue = line.find(": ");
		if (firstKeyValue != std::string::npos)
		{
			auto key = line.substr(0, firstKeyValue);
			auto value = line.substr(firstKeyValue + 2);

			if (node.is_null())
				node = Json::object();

			if (!node.is_object())
				continue;

			if (!value.empty())
			{
				if (value.front() == '"' && value.back() == '"')
				{
					value = value.substr(1, value.size() - 2);
					node[key] = std::move(value);
					continue;
				}

				if (value.front() == '{' && value.back() == '}')
				{
					value = value.substr(1, value.size() - 2);
					node[key] = parseAbbreviatedObject(value);
					continue;
				}

				if (value.front() == '[' && value.back() == ']')
				{
					value = value.substr(1, value.size() - 2);
					node[key] = parseAbbreviatedList(value);
					continue;
				}
			}

			if (String::equals("false", value))
			{
				node[key] = false;
			}
			else if (String::equals("true", value))
			{
				node[key] = true;
			}
			else if (String::equals("null", value))
			{
				node[key] = Json();
			}
			else
			{
				if (!value.empty())
				{
					auto foundInteger = value.find_first_not_of("0123456789");
					if (foundInteger == std::string::npos)
					{
						auto numValue = strtoll(value.c_str(), NULL, 0);
						node[key] = numValue;
						continue;
					}

					auto foundFloat = value.find_first_not_of("0123456789.");
					if (foundFloat == std::string::npos)
					{
						auto firstDecimal = value.find('.');
						if (value.find('.', firstDecimal + 1) == std::string::npos)
						{
							auto numValue = strtof(value.c_str(), NULL);
							node[key] = numValue;
							continue;
						}
					}
				}

				node[key] = std::move(value);
			}
		}
		// Arrays
		else if (!line.empty())
		{
			if (node.is_null())
				node = Json::array();

			if (!node.is_array())
				continue;

			if (line.front() == '"' && line.back() == '"')
				line = line.substr(1, line.size() - 2);

			node.push_back(std::move(line));
		}
	}

	// LOG(outJson.dump(3, ' '));

	return true;
}

/*****************************************************************************/
StringList YamlFile::parseAbbreviatedList(const std::string& inValue) const
{
	auto list = String::split(inValue, ',');
	for (auto& item : list)
	{
		while (!item.empty() && item.front() == ' ')
			item = item.substr(1);

		while (!item.empty() && item.back() == ' ')
			item.pop_back();
	}

	return list;
}

/*****************************************************************************/
Json YamlFile::parseAbbreviatedObject(const std::string& inValue) const
{
	Json ret = Json::object();

	auto list = String::split(inValue, ',');
	for (auto& item : list)
	{
		while (!item.empty() && item.front() == ' ')
			item = item.substr(1);

		while (!item.empty() && item.back() == ' ')
			item.pop_back();

		auto firstKeyValue = item.find(": ");
		if (firstKeyValue != std::string::npos)
		{
			auto key = item.substr(0, firstKeyValue);
			auto value = item.substr(firstKeyValue + 2);
			if (key.empty())
				continue;

			if (!value.empty())
			{
				if (value.front() == '"' && value.back() == '"')
					value = value.substr(1, value.size() - 2);
			}

			ret[key] = std::move(value);
		}
	}

	return ret;
}

/*****************************************************************************/
bool YamlFile::save(const Json& inJson)
{
	auto contents = getNodeAsString(std::string(), inJson);
	// LOG(contents);

	std::ofstream(m_filename) << contents << std::endl;

	return true;
}

/*****************************************************************************/
std::string YamlFile::getNodeAsString(const std::string& inKey, const Json& node, const size_t inIndent, const bool inArray) const
{
	std::string ret(inIndent * 2, ' ');

	bool root = inKey.empty();
	if (inArray)
	{
		// Disables arrays with extra indent
		//
		// if (ret.size() >= 2)
		// {
		// 	ret.pop_back();
		// 	ret.pop_back();
		// }

		ret += "- ";
	}
	else if (!root)
	{
		ret += inKey + ':';
		if (!node.is_array() && !node.is_object())
			ret += ' ';
	}

	if (node.is_array())
	{
		if (!root)
			ret += '\n';

		for (auto& value : node)
		{
			ret += getNodeAsString(std::string(), value, inIndent + 1, true);
		}
	}
	else if (node.is_object())
	{
		if (!root)
			ret += '\n';

		for (auto& [key, value] : node.items())
		{
			size_t indent = root ? 0 : inIndent + 1;
			ret += getNodeAsString(key, value, indent);
		}
	}
	else if (node.is_string())
	{
		auto val = node.get<std::string>();
		if (!val.empty())
		{
			auto foundFloat = val.find_first_not_of("0123456789.");
			auto startsWithAsterisk = val.front() == '*';
			auto startsWithAmpersand = val.front() == '&';
			auto startsWithSquareBrace = val.front() == '[';
			if (foundFloat == std::string::npos || startsWithAsterisk || startsWithAmpersand || startsWithSquareBrace)
			{
				ret += fmt::format("\"{}\"", val);
			}
			else
			{
				ret += std::move(val);
			}
		}
		else
		{
			ret += "\"\"";
		}
		ret += '\n';
	}
	else if (node.is_number_unsigned())
	{
		ret += std::to_string(node.get<u64>());
		ret += '\n';
	}
	else if (node.is_number_integer())
	{
		ret += std::to_string(node.get<i64>());
		ret += '\n';
	}
	else if (node.is_number_float())
	{
		ret += std::to_string(node.get<f64>());
		ret += '\n';
	}
	else if (node.is_boolean())
	{
		bool val = node.get<bool>();
		ret += val ? "true" : "false";
		ret += '\n';
	}
	else if (node.is_null())
	{
		ret += "null";
		ret += '\n';
	}

	return ret;
}
}
