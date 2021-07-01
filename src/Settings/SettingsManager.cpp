/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Settings/SettingsManager.hpp"

#include "BuildJson/SchemaBuildJson.hpp"
#include "Core/CommandLineInputs.hpp"
#include "SettingsJson/SchemaSettingsJson.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsManager::SettingsManager(const CommandLineInputs& inInputs, const SettingsAction inAction) :
	m_inputs(inInputs),
	m_cache(inInputs),
	m_key(inInputs.settingsKey()),
	m_value(inInputs.settingsValue()),
	m_action(inAction),
	m_type(inInputs.settingsType())
{
}

/*****************************************************************************/
bool SettingsManager::run()
{
	if (!m_cache.initialize())
		return false;

	auto& settings = getSettings();
	if (!Commands::pathExists(settings.filename()))
	{
		if (m_type == SettingsType::Global)
			Diagnostic::error("File '{}' doesn't exist.", settings.filename());
		else
			Diagnostic::error("Not a chalet project, or a build hasn't been run yet.", settings.filename());
		return false;
	}

	Json& node = settings.json;
	if (!node.is_object())
	{
		node = Json::object();
		settings.setDirty(true);
	}

	switch (m_action)
	{
		case SettingsAction::Get:
			if (!runSettingsGet(node))
				return false;
			break;

		case SettingsAction::Set:
			if (!runSettingsSet(node))
				return false;
			break;

		case SettingsAction::Unset:
			if (!runSettingsUnset(node))
				return false;
			break;

		default:
			break;
	}

	settings.save();

	return false;
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
		std::cout << value << std::endl;
	}
	else
	{
		std::cout << ptr->dump(3, ' ') << std::endl;
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
	if (ptr->is_string())
	{
		*ptr = m_value;
		set = true;
	}
	else if (ptr->is_boolean())
	{
		if (String::equals({ "true", "1" }, m_value))
		{
			*ptr = true;
			set = true;
		}
		else if (String::equals({ "false", "0" }, m_value))
		{
			*ptr = false;
			set = true;
		}
		else
		{
			Diagnostic::error("'{}' expects a value of true or false.", m_key);
			return false;
		}
	}
	else if (ptr->is_number_integer())
	{
		if (m_value.find_first_not_of("1234567890-") != std::string::npos)
		{
			Diagnostic::error("'{}' expects a signed integer, but found value of '{}'", m_key, m_value);
			return false;
		}
		int value = std::stoi(m_value);
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
		float value = std::stof(m_value);
		*ptr = value;
		set = true;
	}
	else if (ptr->is_object())
	{
		CHALET_TRY
		{
			*ptr = Json::parse(m_value);
			set = true;
		}
		CHALET_CATCH(const std::exception& err)
		{
			CHALET_EXCEPT_ERROR(err.what());
			Diagnostic::error("Couldn't parse value: {}", m_value);
			return false;
		}
	}
	else
	{
		Diagnostic::error("Unknown key: '{}'", m_key);
		return false;
	}

	if (set)
	{
		if (ptr->is_object())
			std::cout << fmt::format("\"{}\": {}", m_key, ptr->dump(3, ' ')) << std::endl;
		else
			std::cout << fmt::format("{}: {}", m_key, m_value) << std::endl;

		settings.setDirty(true);
	}

	if (validate)
	{
		if (String::endsWith(settings.filename(), ".chaletrc"))
		{
			Json schema = Schema::getSettingsJson();
			if (!settings.validate(std::move(schema)))
			{
				settings.setDirty(false);
				return false;
			}
		}
		else if (String::endsWith(settings.filename(), m_inputs.inputFile()))
		{
			Json schema = Schema::getBuildJson();
			if (!settings.validate(std::move(schema)))
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
	auto keySplit = String::split(m_key, '.');
	for (auto& subKey : keySplit)
	{
		std::ptrdiff_t i = &subKey - &keySplit.front();
		bool notLast = i < static_cast<std::ptrdiff_t>(keySplit.size() - 1);
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
			std::size_t val = static_cast<std::size_t>(std::stoi(idxRaw));
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
		std::size_t val = static_cast<std::size_t>(std::stoi(idxRaw));
		ptr->erase(val);
	}
	else
	{
		if (!ptr->contains(lastKey))
			return onNotFound(lastKey);

		ptr->erase(lastKey);
	}
	settings.setDirty(true);

	std::cout << fmt::format("unset: {}", m_key) << std::endl;

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
		bool fail = false;
		StringList keySplit = String::split(m_key, '.');
		std::string idxRaw;
		std::string subKeyRaw;
		for (auto& subKey : keySplit)
		{
			if (!getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw))
			{
				fail = true;
			}

			if (!outNode->contains(subKey))
			{
				fail = true;
			}

			if (fail)
			{
				auto& key = !subKeyRaw.empty() ? subKeyRaw : subKey;
				auto loc = m_key.find(key);
				outLastKey = m_key.substr(0, loc + key.size());
				return false;
			}

			outNode = &outNode->at(subKey);

			if (outNode->is_array() && !idxRaw.empty())
			{
				std::size_t val = static_cast<std::size_t>(std::stoi(idxRaw));
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
	auto keySplit = String::split(m_key, '.');
	for (auto& subKey : keySplit)
	{
		std::ptrdiff_t i = &subKey - &keySplit.front();
		bool notLast = i < static_cast<std::ptrdiff_t>(keySplit.size() - 1);
		getArrayKeyWithIndex(subKey, subKeyRaw, idxRaw);

		if (!outNode->contains(subKey) && notLast)
		{
			(*outNode)[subKey] = Json::object();
		}

		if (!idxRaw.empty())
		{
			outNode = &outNode->at(subKey);
			LOG("idxRaw:", idxRaw);
			std::size_t val = static_cast<std::size_t>(std::stoi(idxRaw));
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
	else if (String::equals({ "true", "false", "0", "1" }, m_value))
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
		std::size_t val = static_cast<std::size_t>(std::stoi(idxRaw));
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

}
