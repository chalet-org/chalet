/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/QueryController.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Settings/SettingsManager.hpp"
#include "State/StatePrototype.hpp"
#include "Terminal/ColorTheme.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
QueryController::QueryController(const CommandLineInputs& inInputs, const StatePrototype& inPrototype) :
	m_inputs(inInputs),
	m_prototype(inPrototype)
{
}

/*****************************************************************************/
bool QueryController::printListOfRequestedType()
{
	QueryOption query = m_inputs.queryOption();
	if (query == QueryOption::None)
		return false;

	StringList output;
	switch (query)
	{
		case QueryOption::Commands:
			output = m_inputs.commandList();
			break;

		case QueryOption::Configurations:
			output = getBuildConfigurationList();
			break;

		case QueryOption::ToolchainPresets:
			output = m_inputs.getToolchainPresets();
			break;

		case QueryOption::UserToolchains:
			output = getUserToolchainList();
			break;

		case QueryOption::Architectures:
			output = getArchitectures();
			break;

		case QueryOption::QueryNames:
			output = m_inputs.getCliQueryOptions();
			break;

		case QueryOption::ThemeNames:
			output = ColorTheme::presets();
			break;

		case QueryOption::Architecture:
			output = getCurrentArchitecture();
			break;

		case QueryOption::Configuration:
			output = getCurrentBuildConfiguration();
			break;

		case QueryOption::Toolchain:
			output = getCurrentToolchain();
			break;

		case QueryOption::RunTarget:
			output = getCurrentRunTarget();
			break;

		case QueryOption::AllToolchains: {
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
const Json& QueryController::getSettingsJson() const
{
	if (m_prototype.cache.exists(CacheType::Local))
	{
		const auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Local);
		return settingsFile.json;
	}
	else if (m_prototype.cache.exists(CacheType::Global))
	{
		const auto& settingsFile = m_prototype.cache.getSettings(SettingsType::Global);
		return settingsFile.json;
	}

	return kEmptyJson;
}

/*****************************************************************************/
StringList QueryController::getBuildConfigurationList() const
{
	StringList ret;

	const auto defaultBuildConfigurations = BuildConfiguration::getDefaultBuildConfigurationNames();
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
StringList QueryController::getUserToolchainList() const
{
	StringList ret;

	const std::string kKeyToolchains = "toolchains";

	const auto& settingsFile = getSettingsJson();
	if (!settingsFile.is_null())
	{
		if (settingsFile.contains(kKeyToolchains))
		{
			const auto& toolchains = settingsFile.at(kKeyToolchains);
			for (auto& [key, _] : toolchains.items())
			{
				ret.emplace_back(key);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getArchitectures() const
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

/*****************************************************************************/
StringList QueryController::getCurrentArchitecture() const
{
	StringList ret;

	const std::string kKeySettings = "settings";
	const std::string kKeyArchitecture = "architecture";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeySettings))
		{
			const auto& settings = settingsFile.at(kKeySettings);
			if (settings.contains(kKeyArchitecture))
			{
				const auto& arch = settings.at(kKeyArchitecture);
				if (arch.is_string())
				{
					auto value = arch.get<std::string>();
					if (!value.empty())
					{
						ret.emplace_back(std::move(value));
					}
				}
			}
		}
	}

	if (ret.empty())
	{
		ret.emplace_back(m_inputs.defaultArchPreset());
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentBuildConfiguration() const
{
	StringList ret;

	const std::string kKeySettings = "settings";
	const std::string kKeyConfiguration = "configuration";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeySettings))
		{
			const auto& settings = settingsFile.at(kKeySettings);
			if (settings.contains(kKeyConfiguration))
			{
				const auto& configuration = settings.at(kKeyConfiguration);
				if (configuration.is_string())
				{
					auto value = configuration.get<std::string>();
					if (!value.empty())
					{
						ret.emplace_back(std::move(value));
					}
				}
			}
		}
	}

	/*if (ret.empty())
	{
		ret.emplace_back(m_prototype.releaseConfiguration());
	}*/

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentToolchain() const
{
	StringList ret;

	const std::string kKeySettings = "settings";
	const std::string kKeyToolchain = "toolchain";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeySettings))
		{
			const auto& settings = settingsFile.at(kKeySettings);
			if (settings.contains(kKeyToolchain))
			{
				const auto& toolchain = settings.at(kKeyToolchain);
				if (toolchain.is_string())
				{
					auto value = toolchain.get<std::string>();
					if (!value.empty())
					{
						ret.emplace_back(std::move(value));
					}
				}
			}
		}
	}

	if (ret.empty())
	{
		ret.emplace_back(m_inputs.defaultToolchainPreset());
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentRunTarget() const
{
	StringList ret;

	const auto& buildJson = m_prototype.chaletJson().json;

	StringList executableProjects;
	const std::string kKeyTargets{ "targets" };
	if (buildJson.is_object())
	{
		if (buildJson.contains(kKeyTargets))
		{
			const std::string kKeyKind{ "kind" };
			const std::string kKeyRunTarget{ "runTarget" };
			const auto& targets = buildJson.at(kKeyTargets);
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object() || !target.contains(kKeyKind))
					continue;

				const auto& kind = target.at(kKeyKind);
				if (!kind.is_string())
					continue;

				auto kindValue = kind.get<std::string>();
				if (!String::equals({ "executable", "script" }, kindValue))
					continue;

				executableProjects.push_back(key);

				if (!target.contains(kKeyRunTarget))
					continue;

				const auto& runTarget = target.at(kKeyRunTarget);
				if (!runTarget.is_boolean())
					continue;

				bool isRunTarget = runTarget.get<bool>();
				if (isRunTarget)
				{
					ret.emplace_back(key);
					return ret;
				}
			}
		}
	}

	if (!executableProjects.empty())
	{
		ret.emplace_back(executableProjects.front());
	}

	return ret;
}
}
