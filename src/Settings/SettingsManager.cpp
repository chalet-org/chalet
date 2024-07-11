/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Settings/SettingsManager.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"
#include "System/Files.hpp"
#include "Utility/String.hpp"
#include "Yaml/YamlFile.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsManager::SettingsManager(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	m_key(inInputs.settingsKey()),
	m_value(inInputs.settingsValue()),
	m_type(inInputs.settingsType())
{
}

/*****************************************************************************/
bool SettingsManager::run(const SettingsAction inAction)
{

	if (m_type == SettingsType::None)
	{
		Diagnostic::error("There was an error determining the settings request");
		return false;
	}

	m_action = inAction;

	if (!initialize())
		return m_action == SettingsAction::QueryKeys;

	auto& settings = getSettings();
	switch (m_action)
	{
		case SettingsAction::Get:
			if (!runSettingsGet(settings.json))
				return false;
			break;

		case SettingsAction::Set:
			if (!runSettingsSet(settings.json))
				return false;
			break;

		case SettingsAction::Unset:
			if (!runSettingsUnset(settings.json))
				return false;
			break;

		case SettingsAction::QueryKeys:
			if (!runSettingsKeyQuery(settings.json))
				return false;
			break;

		default:
			break;
	}

	settings.save();

	return true;
}

/*****************************************************************************/
bool SettingsManager::initialize()
{
	if (m_initialized)
		return true;

	if (!m_cache.initializeSettings(m_inputs))
		return false;

	// if (!m_cache.initialize())
	// 	return false;

	auto& settings = getSettings();
	auto& filename = settings.filename();
	if (!Files::pathExists(filename))
	{
		if (m_action != SettingsAction::QueryKeys)
		{
			if (m_type == SettingsType::Global)
				Diagnostic::error("File '{}' doesn't exist.", filename);
			else
				Diagnostic::error("Not a chalet project, or a build hasn't been run yet.", filename);
		}
		return false;
	}

	if (String::endsWith(".yaml", filename))
		m_yamlOutput = true;

	Json& node = settings.json;
	if (!node.is_object())
	{
		node = Json::object();
		settings.setDirty(true);
	}

	m_initialized = true;

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsGet(Json& node)
{
	Json* ptr = nullptr;
	if (!findRequestedNodeWithFailure(node, ptr))
		return false;

	if (ptr->is_string())
	{
		auto value = ptr->get<std::string>();
		std::cout.write(value.data(), value.size());
		std::cout.put('\n');
		std::cout.flush();
	}
	else
	{
		std::string output;
		if (m_yamlOutput)
			output = YamlFile::asString(*ptr);
		else
			output = ptr->dump(3, ' ');

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsKeyQuery(Json& node)
{
	StringList keyResultList;

	auto escapeString = [](std::string str) -> std::string {
		String::replaceAll(str, '.', R"(\\.)");
		return str;
	};

	Json* ptr = &node;

	StringList subKeys = parseKey();
	if (subKeys.empty() || !subKeys.back().empty())
		subKeys.push_back(std::string()); // ensures object key w/o dot will get handled

	std::string idxRaw;
	std::string subKeyRaw;
	std::string outKeyPath;
	for (auto& subKey : subKeys)
	{
		if (subKey.empty() || !getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw) || !ptr->contains(subKey))
		{
			if (ptr->is_object())
			{
				for (auto& [key, _] : ptr->items())
				{
					if (outKeyPath.empty())
						keyResultList.emplace_back(escapeString(key));
					else
						keyResultList.emplace_back(fmt::format("{}.{}", outKeyPath, escapeString(key)));
				}
			}
			else
			{
				if (!outKeyPath.empty())
					keyResultList.emplace_back(std::move(outKeyPath));
			}
			break;
		}

		ptr = &ptr->at(subKey);

		if (outKeyPath.empty())
			outKeyPath = escapeString(subKey);
		else
			outKeyPath += fmt::format(".{}", escapeString(subKey));

		if (!ptr->is_object())
			continue;

		if (ptr->is_array() && !idxRaw.empty())
		{
			size_t val = static_cast<size_t>(std::stoi(idxRaw));
			if (val < ptr->size())
			{
				ptr = &(*ptr)[val];
			}
			else
			{
				break;
			}
		}
	}

	if (!keyResultList.empty())
	{
		auto output = String::join(keyResultList);
		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsSet(Json& node)
{
	if (m_key.empty())
	{
		Diagnostic::error("Not found: '{}'", m_key);
		return false;
	}

	bool validate = false;
	Json* ptr = nullptr;
	std::string lastKey;
	if (!findRequestedNode(node, lastKey, ptr))
	{
		if (!makeSetting(node, ptr))
			return false;

		validate = true;
	}

	bool set = false;
	auto& settings = getSettings();

	if (String::startsWith(StringList{ "{", "[" }, m_value))
	{
		CHALET_TRY
		{
			*ptr = Json::parse(m_value);
			set = true;
		}
		CHALET_CATCH(const std::exception& err)
		{
			CHALET_EXCEPT_ERROR(err.what());
			Diagnostic::error("Couldn't parse value: '{}'", m_value);
			return false;
		}
	}
	else if (String::equals("true", m_value))
	{
		*ptr = true;
		set = true;
	}
	else if (String::equals("false", m_value))
	{
		*ptr = false;
		set = true;
	}
	else if (ptr->is_number_integer())
	{
		if (m_value.find_first_not_of("1234567890-") != std::string::npos)
		{
			Diagnostic::error("'{}' expects a signed integer, but found value of '{}'", m_key, m_value);
			return false;
		}
		i32 value = std::stoi(m_value);
		*ptr = value;
		set = true;
	}
	else if (ptr->is_number_float())
	{
		if (m_value.find_first_not_of("1234567890-.") != std::string::npos)
		{
			Diagnostic::error("'{}' expects a float, but found value of '{}'", m_key, m_value);
			return false;
		}
		f32 value = std::stof(m_value);
		*ptr = value;
		set = true;
	}
	else
	{
		// string
		*ptr = m_value;
		set = true;
	}

	if (set)
	{
		std::string key(m_key);
		String::replaceAll(key, "\\.", ".");

		std::string content;
		if (m_yamlOutput)
			content = YamlFile::asString(*ptr);
		else
			content = ptr->dump(3, ' ');

		std::string output;
		if (ptr->is_object())
			output = fmt::format("\"{}\": {}", key, content);
		else
			output = fmt::format("{}: {}", key, m_value);

		std::cout.write(output.data(), output.size());
		std::cout.put('\n');
		std::cout.flush();

		settings.setDirty(true);
	}

	if (validate)
	{
		if (String::endsWith(m_inputs.globalSettingsFile(), settings.filename()))
		{
			doSettingsCorrections(settings.json);
		}
		else if (String::endsWith(m_inputs.defaultSettingsFile(), settings.filename()))
		{
			Json schema = SettingsJsonSchema::get(m_inputs);
			if (!settings.validate(schema))
			{
				settings.setDirty(false);
				return false;
			}
		}
		else if (String::endsWith(m_inputs.defaultInputFile(), settings.filename()))
		{
			// note: settings, but not settings (chalet.json)
			Json schema = ChaletJsonSchema::get(m_inputs);
			if (!settings.validate(schema))
			{
				settings.setDirty(false);
				return false;
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsUnset(Json& node)
{

	auto onNotFound = [](const std::string& inLastKey) -> bool {
		Diagnostic::error("Not found: '{}'", inLastKey);
		return false;
	};

	Json* ptr = &node;
	std::string lastKey;
	std::string idxRaw;
	std::string subKeyRaw;

	StringList subKeys = parseKey();
	for (auto& subKey : subKeys)
	{
		ptrdiff_t i = &subKey - &subKeys.front();
		bool notLast = i < static_cast<ptrdiff_t>(subKeys.size() - 1);
		getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw);

		if (!ptr->contains(subKey) && notLast)
		{
			auto loc = m_key.find(subKey);
			lastKey = m_key.substr(0, loc + subKey.size());
			return onNotFound(lastKey);
		}

		if (!idxRaw.empty())
		{
			ptr = &ptr->at(subKey);
			size_t val = static_cast<size_t>(std::stoi(idxRaw));
			if (val >= ptr->size())
			{
				auto loc = m_key.find(subKey);
				lastKey = m_key.substr(0, loc + subKey.size());
				return onNotFound(lastKey);
			}

			if (notLast)
			{
				ptr = &(*ptr)[val];
				idxRaw.clear();
				subKeyRaw.clear();
			}
		}
		else if (notLast)
		{

			ptr = &ptr->at(subKey);
		}
		lastKey = subKey;
	}

	auto& settings = getSettings();
	if (ptr->is_array())
	{
		size_t val = static_cast<size_t>(std::stoi(idxRaw));
		ptr->erase(val);
	}
	else
	{
		if (!ptr->contains(lastKey))
			return onNotFound(lastKey);

		ptr->erase(lastKey);
	}
	settings.setDirty(true);

	auto output = fmt::format("unset: {}", m_key);

	std::cout.write(output.data(), output.size());
	std::cout.put('\n');
	std::cout.flush();

	return true;
}

/*****************************************************************************/
bool SettingsManager::findRequestedNodeWithFailure(Json& inNode, Json*& outNode)
{
	std::string lastKey;
	if (!findRequestedNode(inNode, lastKey, outNode))
	{
		Diagnostic::error("Not found: '{}'", lastKey);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::findRequestedNode(Json& inNode, std::string& outLastKey, Json*& outNode)
{
	outNode = &inNode;

	if (!m_key.empty())
	{
		StringList subKeys = parseKey();
		std::string idxRaw;
		std::string subKeyRaw;
		for (auto& subKey : subKeys)
		{
			if (!getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw) || !outNode->contains(subKey))
			{
				auto& key = !subKeyRaw.empty() ? subKeyRaw : subKey;
				auto loc = m_key.find(key);
				outLastKey = m_key.substr(0, loc + key.size());
				return false;
			}

			outNode = &outNode->at(subKey);

			if (outNode->is_array() && !idxRaw.empty())
			{
				size_t val = static_cast<size_t>(std::stoi(idxRaw));
				if (val < outNode->size())
				{
					outNode = &(*outNode)[val];
				}
				else
				{
					auto& key = !subKeyRaw.empty() ? subKeyRaw : subKey;
					auto loc = m_key.find(key);
					outLastKey = m_key.substr(0, loc + key.size());
					return false;
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::makeSetting(Json& inNode, Json*& outNode)
{
	outNode = &inNode;

	std::string lastKey;
	std::string idxRaw;
	std::string subKeyRaw;

	StringList subKeys = parseKey();
	for (auto& subKey : subKeys)
	{
		ptrdiff_t i = &subKey - &subKeys.front();
		bool notLast = i < static_cast<ptrdiff_t>(subKeys.size() - 1);
		getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw);

		if (!outNode->contains(subKey) && notLast)
		{
			(*outNode)[subKey] = Json::object();
		}

		if (!idxRaw.empty())
		{
			outNode = &outNode->at(subKey);

			size_t val = static_cast<size_t>(std::stoi(idxRaw));
			while (val >= outNode->size())
			{
				outNode->push_back(nullptr);
			}

			if (notLast)
			{
				outNode = &(*outNode)[val];
				idxRaw.clear();
				subKeyRaw.clear();
			}
		}
		else if (notLast)
		{
			outNode = &outNode->at(subKey);
		}
		lastKey = subKey;
	}

	Json value;

	if (String::startsWith("{", m_value) && String::endsWith("}", m_value))
	{
		value = Json::object();
	}
	else if (String::equals(StringList{ "true", "false", "0", "1" }, m_value))
	{
		value = false;
	}
	else if (m_value.find_first_not_of("1234567890-") == std::string::npos)
	{
		value = 0;
	}
	else if (m_value.find_first_not_of("1234567890-.") == std::string::npos)
	{
		value = 0.0f;
	}
	else
	{
		value = std::string();
	}

	if (outNode->is_array())
	{
		size_t val = static_cast<size_t>(std::stoi(idxRaw));
		if (val < outNode->size())
		{
			(*outNode)[val] = std::move(value);
			outNode = &(*outNode)[val];
		}
	}
	else
	{
		(*outNode)[lastKey] = std::move(value);
		outNode = &outNode->at(lastKey);
	}

	return true;
}

/*****************************************************************************/
JsonFile& SettingsManager::getSettings()
{
	return m_cache.getSettings(m_type);
}

/*****************************************************************************/
void SettingsManager::doSettingsCorrections(Json& node)
{
	// Note: "theme" takes a string or a set of digits
	//   If digits, we have to save them as a string
	//
	bool isTheme = String::equals(Keys::Theme, m_key);
	if (isTheme && node.contains(Keys::Theme))
	{
		auto& theme = node.at(Keys::Theme);
		if (theme.is_number_integer())
		{
			theme = m_value;
		}
	}
}

/*****************************************************************************/
bool SettingsManager::getArrayKeyWithIndex(std::string& inKey, std::string& outRawKey, std::string& outIndex)
{
	auto openBracket = inKey.find('[');
	if (openBracket != std::string::npos)
	{
		auto closeBracket = inKey.find(']', openBracket);
		if (closeBracket != std::string::npos)
		{
			outRawKey = inKey;
			outIndex = inKey.substr(openBracket + 1, closeBracket - (openBracket + 1));
			inKey = inKey.substr(0, openBracket);
			auto valid = outIndex.find_first_not_of("0123456789") == std::string::npos;
			if (!valid)
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
StringList SettingsManager::parseKey() const
{
	StringList ret;
	auto split = String::split(m_key, '.');
	std::string tmp;
	for (auto& key : split)
	{
		if (String::endsWith('\\', key))
		{
			key.pop_back();
			key += ".";
			tmp += key;
		}
		else
		{
			tmp += key;
			ret.push_back(std::move(tmp));
		}
	}
	return ret;
}

}
