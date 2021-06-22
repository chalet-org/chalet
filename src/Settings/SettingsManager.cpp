/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Settings/SettingsManager.hpp"

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

	auto& config = m_type == SettingsType::Global ? m_cache.globalConfig() : m_cache.localConfig();
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

		default:
			break;
	}

	config.save();

	return false;
}

/*****************************************************************************/
bool SettingsManager::runSettingsGet(Json& node)
{
	if (!m_key.empty())
	{
		auto keySplit = String::split(m_key, '.');
		for (auto& subKey : keySplit)
		{
			std::ptrdiff_t i = &subKey - &keySplit.front();
			if (!node.contains(subKey))
			{
				auto loc = m_key.find(subKey);
				std::string out = m_key.substr(0, loc + subKey.size());
				Diagnostic::error("Not found: '{}'", out);
				return false;
			}

			if (i < static_cast<std::ptrdiff_t>(keySplit.size()))
			{
				node = node.at(subKey);
			}
		}
	}

	if (node.is_string())
	{
		auto value = node.get<std::string>();
		std::cout << value << std::endl;
	}
	else
	{
		std::cout << node.dump(3, ' ') << std::endl;
	}

	return true;
}

/*****************************************************************************/
bool SettingsManager::runSettingsSet(Json& node)
{
	UNUSED(node);
	return true;
}
}
