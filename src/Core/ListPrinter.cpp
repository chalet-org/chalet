/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ListPrinter.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Settings/SettingsManager.hpp"
#include "State/StatePrototype.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ListPrinter::ListPrinter(const CommandLineInputs& inInputs, const StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool ListPrinter::printListOfRequestedType()
{
	CommandLineListOption listOption = m_inputs.listOption();
	if (listOption == CommandLineListOption::None)
		return false;

	StringList output;
	switch (listOption)
	{
		case CommandLineListOption::Commands:
			output = m_inputs.commandList();
			break;

		case CommandLineListOption::Configurations:
			output = getBuildConfigurationList();
			break;

		case CommandLineListOption::ToolchainPresets:
			output = m_inputs.getToolchainPresets();
			break;

		case CommandLineListOption::UserToolchains:
			output = getUserToolchainList();
			break;

		case CommandLineListOption::Architectures:
			output = getArchitectures();
			break;

		case CommandLineListOption::ListNames:
			output = m_inputs.getCommandLineListNames();
			break;

		case CommandLineListOption::AllToolchains: {
			StringList presets = m_inputs.getToolchainPresets();
			StringList userToolchains = getUserToolchainList();
			output = List::combine(std::move(userToolchains), std::move(presets));
			break;
		}

		default:
			break;
	}

	std::cout << String::join(output) << std::endl;

	return true;
}

/*****************************************************************************/
StringList ListPrinter::getBuildConfigurationList() const
{
	StringList ret;

	const auto& defaultBuildConfigurations = m_prototype.defaultBuildConfigurations();
	const auto& buildJson = m_prototype.chaletJson().json;

	const std::string kKeyConfigurations = "configurations";

	if (!buildJson.contains(kKeyConfigurations))
	{
		return defaultBuildConfigurations;
	}

	StringList buildConfigurations;
	const Json& configurations = buildJson.at(kKeyConfigurations);
	if (configurations.is_object())
	{
		for (auto& [name, configJson] : configurations.items())
		{
			if (!configJson.is_object() || name.empty())
				continue;

			buildConfigurations.emplace_back(name);
		}
	}
	else if (configurations.is_array())
	{
		for (auto& configJson : configurations)
		{
			if (configJson.is_string())
			{
				auto name = configJson.get<std::string>();
				if (name.empty() || !List::contains(defaultBuildConfigurations, name))
					continue;

				ret.emplace_back(std::move(name));
			}
		}

		return ret;
	}

	if (!buildConfigurations.empty())
	{
		StringList defaults;
		StringList userDefined;
		for (const auto& name : buildConfigurations)
		{
			if (List::contains(defaultBuildConfigurations, name))
				defaults.push_back(name);
			else
				userDefined.emplace_back(name);
		}

		// Order as defaults first, user defined second
		if (!defaults.empty())
		{
			for (auto& name : defaultBuildConfigurations)
			{
				if (List::contains(defaults, name))
					ret.emplace_back(name);
			}
		}

		for (auto&& name : userDefined)
		{
			ret.emplace_back(std::move(name));
		}
	}
	else
	{
		return defaultBuildConfigurations;
	}

	return ret;
}

/*****************************************************************************/
StringList ListPrinter::getUserToolchainList() const
{
	StringList ret;

	const std::string kKeyToolchains = "toolchains";

	auto parseToolchains = [&](const Json& inNode, StringList& outList) {
		if (!inNode.contains(kKeyToolchains))
			return;

		const auto& toolchains = inNode.at(kKeyToolchains);
		for (auto& [key, _] : toolchains.items())
		{
			outList.emplace_back(key);
		}
	};

	if (m_prototype.cache.exists(CacheType::Local))
	{
		auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Local);
		parseToolchains(settingsFile.json, ret);
	}
	else if (m_prototype.cache.exists(CacheType::Global))
	{
		auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Global);
		parseToolchains(settingsFile.json, ret);
	}

	return ret;
}

/*****************************************************************************/
StringList ListPrinter::getArchitectures() const
{
	StringList ret{ "auto" };

#if defined(CHALET_MACOS)
	ret.emplace_back("universal");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm64");
#elif defined(CHALET_WIN32)
	ret.emplace_back("x64");
	ret.emplace_back("x64_x86");
	ret.emplace_back("x64_arm");
	ret.emplace_back("x64_arm64");
	ret.emplace_back("x86_x64");
	ret.emplace_back("x86");
	ret.emplace_back("x86_arm");
	ret.emplace_back("x86_arm64");

	ret.emplace_back("i686");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm");
	ret.emplace_back("arm64");
#else
	ret.emplace_back("i686");
	ret.emplace_back("x86_64");
	ret.emplace_back("arm");
	ret.emplace_back("arm64");
#endif

	return ret;
}
}
