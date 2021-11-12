/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/QueryController.hpp"

#include "BuildJson/SchemaBuildJson.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/SchemaSettingsJson.hpp"
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

		case QueryOption::ChaletJsonState:
			output = getChaletJsonState();
			break;

		case QueryOption::SettingsJsonState:
			output = getSettingsJsonState();
			break;

		case QueryOption::ChaletSchema:
			output = getChaletSchema();
			break;

		case QueryOption::SettingsSchema:
			output = getSettingsSchema();
			break;

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

	auto defaultBuildConfigurations = BuildConfiguration::getDefaultBuildConfigurationNames();
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

	if (buildConfigurations.empty())
	{
		return defaultBuildConfigurations;
	}

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
	const auto& queryData = m_inputs.queryData();
	if (!queryData.empty())
	{
		const auto& toolchain = queryData.front();
		return getArchitectures(toolchain);
	}
	else
	{
		return {
			"auto",
		};
	}
}

/*****************************************************************************/
StringList QueryController::getArchitectures(const std::string& inToolchain) const
{
	std::string kAuto{ "auto" };

	// TODO: Link these up with the toolchain presets declared in CommandLineInputs

	if (String::equals("llvm", inToolchain))
	{
		return {
			std::move(kAuto),
			"x86_64",
			"i686",
			"arm",
			"arm64",
		};
	}
#if defined(CHALET_MACOS)
	else if (String::equals("apple-llvm", inToolchain))
	{
		return {
			std::move(kAuto),
			"universal",
			"x86_64",
			"arm64",
		};
	}
#endif
	else if (String::equals("gcc", inToolchain))
	{
#if defined(CHALET_WIN32)
		return {
			std::move(kAuto),
			"x86_64",
			"i686",
		};
#else
		return {
			std::move(kAuto),
			m_inputs.hostArchitecture(),
		};
#endif
	}
#if defined(CHALET_WIN32)
	else if (String::startsWith("vs-", inToolchain))
	{
		return {
			std::move(kAuto),
			"x64",
			"x64_x86",
			"x64_arm",
			"x64_arm64",
			"x86_x64",
			"x86",
			"x86_arm",
			"x86_arm64",
		};
	}
#endif
#if defined(CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC)
	else if (String::startsWith("intel-classic", inToolchain))
	{
		return
		{
			std::move(kAuto),
				"x86_64",
	#if !defined(CHALET_MACOS)
				"i686",
	#endif
		};
	}
#endif
#if defined(CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX)
	else if (String::startsWith("intel-llvm", inToolchain))
	{
		return {
			std::move(kAuto),
			"x86_64",
			"i686",
		};
	}
#endif

	return {
		std::move(kAuto),
	};
}

/*****************************************************************************/
StringList QueryController::getCurrentArchitecture() const
{
	StringList ret;

	const std::string kKeyOptions = "options";
	const std::string kKeyArchitecture = "architecture";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeyOptions))
		{
			const auto& settings = settingsFile.at(kKeyOptions);
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

	const std::string kKeyOptions = "options";
	const std::string kKeyConfiguration = "configuration";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeyOptions))
		{
			const auto& settings = settingsFile.at(kKeyOptions);
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

	const std::string kKeyOptions = "options";
	const std::string kKeyToolchain = "toolchain";

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(kKeyOptions))
		{
			const auto& settings = settingsFile.at(kKeyOptions);
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
				if (!String::equals({ "executable", "script", "cmakeProject" }, kindValue))
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

/*****************************************************************************/
StringList QueryController::getChaletJsonState() const
{
	StringList ret;

	Json output = Json::object();
	output["configurations"] = getBuildConfigurationList();

	auto runTargetRes = getCurrentRunTarget();
	if (!runTargetRes.empty())
		output["runTarget"] = std::move(runTargetRes.front());

	ret.emplace_back(output.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getSettingsJsonState() const
{
	StringList ret;

	Json output = Json::object();
	auto toolchainPresets = m_inputs.getToolchainPresets();
	auto userToolchains = getUserToolchainList();
	output["allToolchains"] = List::combine(userToolchains, toolchainPresets);
	output["toolchainPresets"] = std::move(toolchainPresets);
	output["userToolchains"] = std::move(userToolchains);

	auto archRes = getCurrentArchitecture();
	if (!archRes.empty())
		output["architecture"] = std::move(archRes.front());

	auto configRes = getCurrentBuildConfiguration();
	if (!configRes.empty())
		output["configuration"] = std::move(configRes.front());

	auto toolchainRes = getCurrentToolchain();
	if (!toolchainRes.empty())
	{
		auto toolchain = std::move(toolchainRes.front());
		output["architectures"] = getArchitectures(toolchain);
		output["toolchain"] = std::move(toolchain);
	}

	ret.emplace_back(output.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getChaletSchema() const
{
	StringList ret;

	SchemaBuildJson schemaBuilder;
	Json schema = schemaBuilder.get();
	ret.emplace_back(schema.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getSettingsSchema() const
{
	StringList ret;

	SchemaSettingsJson schemaBuilder;
	Json schema = schemaBuilder.get();
	ret.emplace_back(schema.dump());

	return ret;
}
}
