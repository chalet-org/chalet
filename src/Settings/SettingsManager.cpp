/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Settings/SettingsManager.hpp"

#include "CacheJson/CacheJsonSchema.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
SettingsManager::SettingsManager(const CommandLineInputs& inInputs, const SettingsAction inAction) :
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

	auto& config = getConfig();
	if (!Commands::pathExists(config.filename()))
	{
		if (m_type == SettingsType::Global)
			Diagnostic::error("File '{}' doesn't exist.", config.filename());
		else
			Diagnostic::error("Not a chalet project, or a build hasn't been run yet.", config.filename());
		return false;
	}

	Json& node = config.json;
	if (!node.is_object())
	{
		node = Json::object();
		config.setDirty(true);
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

	config.save();

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
	auto& config = getConfig();
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
		try
		{
			*ptr = Json::parse(m_value);
			set = true;
		}
		catch (const std::exception& err)
		{
			Diagnostic::error(err.what());
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

		config.setDirty(true);
	}

	if (validate)
	{
		Json cacheJsonSchema = m_type == SettingsType::Global ? Schema::getGlobalConfigJson() : Schema::getCacheJson();
		if (!config.validate(std::move(cacheJsonSchema)))
		{
			config.setDirty(false);
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsUnset(Json& node)
{
	Json* ptr = &node;
	std::string lastKey;
	auto keySplit = String::split(m_key, '.');
	for (auto& subKey : keySplit)
	{
		std::ptrdiff_t i = &subKey - &keySplit.front();
		bool notLast = i < static_cast<std::ptrdiff_t>(keySplit.size() - 1);
		if (!ptr->contains(subKey) && notLast)
		{
			auto loc = m_key.find(subKey);
			lastKey = m_key.substr(0, loc + subKey.size());
			return false;
		}

		if (notLast)
		{
			ptr = &ptr->at(subKey);
		}
		lastKey = subKey;
	}

	auto& config = getConfig();
	ptr->erase(lastKey);
	config.setDirty(true);

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
		auto keySplit = String::split(m_key, '.');
		for (auto& subKey : keySplit)
		{
			if (!outNode->contains(subKey))
			{
				auto loc = m_key.find(subKey);
				outLastKey = m_key.substr(0, loc + subKey.size());
				return false;
			}

			outNode = &outNode->at(subKey);
		}
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::makeSetting(Json& inNode, Json*& outNode)
{
	outNode = &inNode;

	std::string lastKey;
	auto keySplit = String::split(m_key, '.');
	for (auto& subKey : keySplit)
	{
		std::ptrdiff_t i = &subKey - &keySplit.front();
		bool notLast = i < static_cast<std::ptrdiff_t>(keySplit.size() - 1);
		if (!outNode->contains(subKey) && notLast)
		{
			(*outNode)[subKey] = Json::object();
		}

		if (notLast)
		{
			outNode = &outNode->at(subKey);
		}
		lastKey = subKey;
	}

	if (String::startsWith("{", m_value) && String::endsWith("}", m_value))
	{
		(*outNode)[lastKey] = Json::object();
	}
	else if (String::equals({ "true", "false", "0", "1" }, m_value))
	{
		(*outNode)[lastKey] = false;
	}
	else if (m_value.find_first_not_of("1234567890-") == std::string::npos)
	{
		(*outNode)[lastKey] = 0;
	}
	else if (m_value.find_first_not_of("1234567890-.") == std::string::npos)
	{
		(*outNode)[lastKey] = 0.0f;
	}
	else
	{
		(*outNode)[lastKey] = std::string();
	}

	outNode = &outNode->at(lastKey);

	return true;
}

/*****************************************************************************/
JsonFile& SettingsManager::getConfig()
{
	return m_type == SettingsType::Global ? m_cache.globalConfig() : m_cache.localConfig();
}
}
