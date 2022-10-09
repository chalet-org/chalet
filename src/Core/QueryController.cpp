/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/QueryController.hpp"

#include "Arguments/ArgumentParser.hpp"
#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/Arch.hpp"
#include "Core/CommandLineInputs.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "Terminal/ColorTheme.hpp"
#include "Utility/DefinesVersion.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"

namespace chalet
{
/*****************************************************************************/
QueryController::QueryController(const CentralState& inCentralState) :
	m_centralState(inCentralState)
{
}

/*****************************************************************************/
bool QueryController::printListOfRequestedType()
{
	QueryOption query = m_centralState.inputs().queryOption();
	if (query == QueryOption::None)
	{
		Diagnostic::error("Unrecogized query.");
		return false;
	}

	StringList output = getRequestedType(query);

	auto result = String::join(output, '\t');
	std::cout.write(result.data(), result.size());
	std::cout.put('\n');
	std::cout.flush();

	return true;
}

/*****************************************************************************/
StringList QueryController::getRequestedType(const QueryOption inOption) const
{
	StringList ret;

	switch (inOption)
	{
		case QueryOption::Commands:
			ret = m_centralState.inputs().commandList();
			break;

		case QueryOption::Version:
			ret = getVersion();
			break;

		case QueryOption::Configurations:
			ret = getBuildConfigurationList();
			break;

		case QueryOption::Arguments:
			ret = getArguments();
			break;

		case QueryOption::ToolchainPresets:
			ret = m_centralState.inputs().getToolchainPresets();
			break;

		case QueryOption::UserToolchains:
			ret = getUserToolchainList();
			break;

		case QueryOption::Architectures:
			ret = getArchitectures();
			break;

		case QueryOption::ExportKinds:
			ret = m_centralState.inputs().getExportKindPresets();
			break;

		case QueryOption::QueryNames:
			ret = m_centralState.inputs().getCliQueryOptions();
			break;

		case QueryOption::ThemeNames:
			ret = ColorTheme::presets();
			break;

		case QueryOption::Architecture:
			ret = getCurrentArchitecture();
			break;

		case QueryOption::Configuration:
			ret = getCurrentBuildConfiguration();
			break;

		case QueryOption::Toolchain:
			ret = getCurrentToolchain();
			break;

		case QueryOption::RunTarget:
			ret = getCurrentRunTarget();
			break;

		case QueryOption::AllRunTargets:
			ret = getAllRunTargets();
			break;

		case QueryOption::AllToolchains: {
			StringList presets = m_centralState.inputs().getToolchainPresets();
			StringList userToolchains = getUserToolchainList();
			ret = List::combine(std::move(userToolchains), std::move(presets));
			break;
		}

		case QueryOption::BuildStrategy:
			ret = getCurrentToolchainBuildStrategy();
			break;

		case QueryOption::BuildStrategies:
			ret = getToolchainBuildStrategies();
			break;

		case QueryOption::BuildPathStyle:
			ret = getCurrentToolchainBuildPathStyle();
			break;

		case QueryOption::BuildPathStyles:
			ret = getToolchainBuildPathStyles();
			break;

		case QueryOption::ChaletJsonState:
			ret = getChaletJsonState();
			break;

		case QueryOption::SettingsJsonState:
			ret = getSettingsJsonState();
			break;

		case QueryOption::ChaletSchema:
			ret = getChaletSchema();
			break;

		case QueryOption::SettingsSchema:
			ret = getSettingsSchema();
			break;

		case QueryOption::None:
		default:
			break;
	}

	return ret;
}

/*****************************************************************************/
const Json& QueryController::getSettingsJson() const
{
	if (m_centralState.cache.exists(CacheType::Local))
	{
		const auto& settingsFile = m_centralState.cache.getSettings(SettingsType::Local);
		return settingsFile.json;
	}
	else if (m_centralState.cache.exists(CacheType::Global))
	{
		const auto& settingsFile = m_centralState.cache.getSettings(SettingsType::Global);
		return settingsFile.json;
	}

	return kEmptyJson;
}

/*****************************************************************************/
StringList QueryController::getVersion() const
{
	return {
		std::string(CHALET_VERSION),
	};
}

/*****************************************************************************/
StringList QueryController::getBuildConfigurationList() const
{
	StringList ret;

	auto defaultBuildConfigurations = BuildConfiguration::getDefaultBuildConfigurationNames();
	const auto& chaletJson = m_centralState.chaletJson().json;

	bool addedDefaults = false;
	if (chaletJson.contains(Keys::DefaultConfigurations))
	{
		const Json& defaultConfigurations = chaletJson.at(Keys::DefaultConfigurations);
		if (defaultConfigurations.is_array())
		{
			addedDefaults = true;
			for (auto& configJson : defaultConfigurations)
			{
				if (configJson.is_string())
				{
					auto name = configJson.get<std::string>();
					if (name.empty() || !List::contains(defaultBuildConfigurations, name))
						continue;

					ret.emplace_back(std::move(name));
				}
			}
		}
	}

	if (!addedDefaults)
	{
		ret = defaultBuildConfigurations;
	}

	if (chaletJson.contains(Keys::Configurations))
	{
		const Json& configurations = chaletJson.at(Keys::Configurations);
		if (configurations.is_object())
		{
			for (auto& [name, configJson] : configurations.items())
			{
				if (!configJson.is_object() || name.empty())
					continue;

				List::addIfDoesNotExist(ret, name);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getUserToolchainList() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (!settingsFile.is_null())
	{
		if (settingsFile.contains(Keys::Toolchains))
		{
			const auto& toolchains = settingsFile.at(Keys::Toolchains);
			for (auto& [key, _] : toolchains.items())
			{
				ret.emplace_back(key);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentToolchainBuildStrategy() const
{
	StringList ret;

	auto maybeToolchain = getCurrentToolchain();
	if (!maybeToolchain.empty())
	{
		const auto& toolchain = maybeToolchain.front();
		const auto& settingsFile = getSettingsJson();
		if (settingsFile.is_object())
		{
			if (settingsFile.contains(Keys::Toolchains))
			{
				const auto& toolchains = settingsFile.at(Keys::Toolchains);
				if (toolchains.contains(toolchain))
				{
					const auto& toolchainJson = toolchains.at(toolchain);
					if (toolchainJson.is_object() && toolchainJson.contains(Keys::ToolchainBuildStrategy))
					{
						const auto& strategy = toolchainJson.at(Keys::ToolchainBuildStrategy);
						if (strategy.is_string())
						{
							auto value = strategy.get<std::string>();
							if (!value.empty())
							{
								ret.emplace_back(std::move(value));
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getToolchainBuildStrategies() const
{
	return CompilerTools::getToolchainStrategies();
}

/*****************************************************************************/
StringList QueryController::getCurrentToolchainBuildPathStyle() const
{
	StringList ret;

	auto maybeToolchain = getCurrentToolchain();
	if (!maybeToolchain.empty())
	{
		const auto& toolchain = maybeToolchain.front();
		const auto& settingsFile = getSettingsJson();
		if (settingsFile.is_object())
		{
			if (settingsFile.contains(Keys::Toolchains))
			{
				const auto& toolchains = settingsFile.at(Keys::Toolchains);
				if (toolchains.contains(toolchain))
				{
					const auto& toolchainJson = toolchains.at(toolchain);
					if (toolchainJson.is_object() && toolchainJson.contains(Keys::ToolchainBuildPathStyle))
					{
						const auto& style = toolchainJson.at(Keys::ToolchainBuildPathStyle);
						if (style.is_string())
						{
							auto value = style.get<std::string>();
							if (!value.empty())
							{
								ret.emplace_back(std::move(value));
							}
						}
					}
				}
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getToolchainBuildPathStyles() const
{
	return CompilerTools::getToolchainBuildPathStyles();
}

/*****************************************************************************/
StringList QueryController::getArchitectures() const
{
	const auto& queryData = m_centralState.inputs().queryData();
	if (!queryData.empty())
	{
		const auto& toolchain = queryData.front();
		return getArchitectures(toolchain);
	}
	else
	{
		auto toolchainList = getCurrentToolchain();
		if (!toolchainList.empty())
		{
			auto& toolchain = toolchainList.front();
			return getArchitectures(toolchain);
		}
	}

	return {
		"auto",
	};
}

/*****************************************************************************/
StringList QueryController::getArchitectures(const std::string& inToolchain) const
{
	StringList ret{ "auto" };

	// TODO: Link these up with the toolchain presets declared in CommandLineInputs

	if (String::startsWith("llvm-", inToolchain))
	{
		List::addIfDoesNotExist(ret, "x86_64");
		List::addIfDoesNotExist(ret, "i686");
		List::addIfDoesNotExist(ret, "arm");
		List::addIfDoesNotExist(ret, "arm64");
	}
#if defined(CHALET_MACOS)
	else if (String::equals("apple-llvm", inToolchain))
	{
		List::addIfDoesNotExist(ret, "universal");
		List::addIfDoesNotExist(ret, "x86_64");
		List::addIfDoesNotExist(ret, "arm64");
	}
#endif
	else if (String::equals("gcc", inToolchain))
	{
#if defined(CHALET_WIN32)
		List::addIfDoesNotExist(ret, "x86_64");
		List::addIfDoesNotExist(ret, "i686");
#else
		List::addIfDoesNotExist(ret, m_centralState.inputs().hostArchitecture());
#endif
	}
#if defined(CHALET_WIN32)
	else if (String::startsWith("vs-", inToolchain))
	{
		if (String::equals("arm64", m_centralState.inputs().hostArchitecture()))
		{
			List::addIfDoesNotExist(ret, "arm64");
			List::addIfDoesNotExist(ret, "arm64_arm64");
			List::addIfDoesNotExist(ret, "arm64_x64");
			List::addIfDoesNotExist(ret, "arm64_x86");
		}
		else
		{
			List::addIfDoesNotExist(ret, "x86_64");
			List::addIfDoesNotExist(ret, "i686");
			List::addIfDoesNotExist(ret, "x64");
			List::addIfDoesNotExist(ret, "x86");
			List::addIfDoesNotExist(ret, "x64_x64");
			List::addIfDoesNotExist(ret, "x64_x86");
			List::addIfDoesNotExist(ret, "x64_arm");
			List::addIfDoesNotExist(ret, "x64_arm64");
			List::addIfDoesNotExist(ret, "x86_x86");
			List::addIfDoesNotExist(ret, "x86_x64");
			List::addIfDoesNotExist(ret, "x86_arm");
			List::addIfDoesNotExist(ret, "x86_arm64");
		}
	}
#endif
#if defined(CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC)
	else if (String::startsWith("intel-classic", inToolchain))
	{
		List::addIfDoesNotExist(ret, "x86_64");
	#if !defined(CHALET_MACOS)
		List::addIfDoesNotExist(ret, "i686");
	#endif
	}
#endif
#if defined(CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX)
	else if (String::startsWith("intel-llvm", inToolchain))
	{
		List::addIfDoesNotExist(ret, "x86_64");
		List::addIfDoesNotExist(ret, "i686");
	}
#endif

	auto currentArch = getCurrentArchitecture();
	if (!currentArch.empty())
	{
		List::addIfDoesNotExist(ret, std::move(currentArch.front()));
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getArguments() const
{
	ArgumentParser parser(m_centralState.inputs());
	return parser.getAllCliOptions();
}

/*****************************************************************************/
StringList QueryController::getCurrentArchitecture() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(Keys::Options))
		{
			const auto& options = settingsFile.at(Keys::Options);
			if (options.is_object() && options.contains(Keys::OptionsArchitecture))
			{
				const auto& arch = options.at(Keys::OptionsArchitecture);
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
		ret.emplace_back(m_centralState.inputs().defaultArchPreset());
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentBuildConfiguration() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(Keys::Options))
		{
			const auto& options = settingsFile.at(Keys::Options);
			if (options.is_object() && options.contains(Keys::OptionsBuildConfiguration))
			{
				const auto& configuration = options.at(Keys::OptionsBuildConfiguration);
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

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentToolchain() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(Keys::Options))
		{
			const auto& options = settingsFile.at(Keys::Options);
			if (options.is_object() && options.contains(Keys::OptionsToolchain))
			{
				const auto& toolchain = options.at(Keys::OptionsToolchain);
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
		ret.emplace_back(m_centralState.inputs().defaultToolchainPreset());
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getAllBuildTargets() const
{
	StringList ret;

	const auto& chaletJson = m_centralState.chaletJson().json;

	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson.at(Keys::Targets);
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object() || !target.contains(Keys::Kind))
					continue;

				const auto& kind = target.at(Keys::Kind);
				if (!kind.is_string())
					continue;

				ret.push_back(key);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getAllRunTargets() const
{
	StringList ret;

	const auto& chaletJson = m_centralState.chaletJson().json;

	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson.at(Keys::Targets);
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object() || !target.contains(Keys::Kind))
					continue;

				const auto& kind = target.at(Keys::Kind);
				if (!kind.is_string())
					continue;

				auto kindValue = kind.get<std::string>();
				if (!String::equals(StringList{ "executable", "script", "cmakeProject" }, kindValue))
					continue;

				bool isExecutable = true;
				if (String::equals("cmakeProject", kindValue))
				{
					if (!target.contains(Keys::RunExecutable))
						isExecutable = false;
				}

				if (isExecutable)
					ret.push_back(key);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getCurrentRunTarget() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(Keys::Options))
		{
			const auto& options = settingsFile.at(Keys::Options);
			if (options.is_object() && options.contains(Keys::OptionsRunTarget))
			{
				const auto& runTarget = options.at(Keys::OptionsRunTarget);
				if (runTarget.is_string())
				{
					auto value = runTarget.get<std::string>();
					if (!value.empty())
					{
						ret.emplace_back(std::move(value));
					}
				}
			}
		}
	}

	if (!ret.empty())
		return ret;

	const auto& chaletJson = m_centralState.chaletJson().json;

	StringList executableProjects;
	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson.at(Keys::Targets);
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object() || !target.contains(Keys::Kind))
					continue;

				const auto& kind = target.at(Keys::Kind);
				if (!kind.is_string())
					continue;

				auto kindValue = kind.get<std::string>();
				if (!String::equals(StringList{ "executable", "script", "cmakeProject" }, kindValue))
					continue;

				bool isExecutable = true;
				if (String::equals("cmakeProject", kindValue))
				{
					if (!target.contains(Keys::RunExecutable))
						isExecutable = false;
				}

				if (isExecutable)
					executableProjects.push_back(key);
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
	output["configurationDetails"] = getBuildConfigurationDetails();
	auto runTargetRes = getCurrentRunTarget();
	if (!runTargetRes.empty())
		output["defaultRunTarget"] = std::move(runTargetRes.front());

	output["runTargets"] = getAllRunTargets();
	output["targets"] = getAllBuildTargets();

	ret.emplace_back(output.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getSettingsJsonState() const
{
	StringList ret;

	Json output = Json::object();
	auto toolchainPresets = m_centralState.inputs().getToolchainPresets();
	auto userToolchains = getUserToolchainList();
	output["allToolchains"] = List::combine(userToolchains, toolchainPresets);
	auto archRes = getCurrentArchitecture();
	if (!archRes.empty())
		output["architecture"] = std::move(archRes.front());
	output["architectures"] = Json::array();

	auto bpathRes = getCurrentToolchainBuildPathStyle();
	if (!bpathRes.empty())
		output["buildPathStyle"] = std::move(bpathRes.front());

	output["buildPathStyles"] = getToolchainBuildPathStyles();
	auto bstratRes = getCurrentToolchainBuildStrategy();
	if (!bstratRes.empty())
		output["buildStrategy"] = std::move(bstratRes.front());

	output["buildStrategies"] = getToolchainBuildStrategies();

	auto configRes = getCurrentBuildConfiguration();
	if (!configRes.empty())
		output["configuration"] = std::move(configRes.front());

	output["toolchain"] = std::string();
	output["toolchainPresets"] = std::move(toolchainPresets);
	output["userToolchains"] = std::move(userToolchains);

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
Json QueryController::getBuildConfigurationDetails() const
{
	Json ret = Json::object();

	Dictionary<Json> configMap;

	const auto& chaletJson = m_centralState.chaletJson().json;
	if (chaletJson.contains(Keys::Configurations))
	{
		const Json& configurations = chaletJson.at(Keys::Configurations);
		if (configurations.is_object())
		{
			for (const auto& [name, config] : configurations.items())
			{
				if (configurations.contains(name))
					configMap[name] = config;
			}
		}
	}

	auto configNames = getBuildConfigurationList();
	for (const auto& name : configNames)
	{
		if (configMap.find(name) != configMap.end())
		{
			ret[name] = std::move(configMap.at(name));
		}
		else
		{
			BuildConfiguration data;
			if (BuildConfiguration::makeDefaultConfiguration(data, name))
			{
				Json conf = Json::object();
				conf["debugSymbols"] = data.debugSymbols();
				conf["enableProfiling"] = data.enableProfiling();
				conf["interproceduralOptimization"] = data.interproceduralOptimization();
				conf["optimizationLevel"] = data.optimizationLevelString();

				auto sans = data.getSanitizerList();
				if (sans.empty())
					conf["sanitize"] = false;
				else
					conf["sanitize"] = std::move(sans);

				ret[name] = std::move(conf);
			}
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getChaletSchema() const
{
	StringList ret;

	ChaletJsonSchema schemaBuilder;
	Json schema = schemaBuilder.get();
	ret.emplace_back(schema.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getSettingsSchema() const
{
	StringList ret;

	SettingsJsonSchema schemaBuilder;
	Json schema = schemaBuilder.get();
	ret.emplace_back(schema.dump());

	return ret;
}
}
