/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Query/QueryController.hpp"

#include "ChaletJson/ChaletJsonSchema.hpp"
#include "Core/Arguments/ArgumentParser.hpp"
#include "Core/CommandLineInputs.hpp"
#include "DotEnv/DotEnvFileParser.hpp"
#include "Platform/Arch.hpp"
#include "Process/Environment.hpp"
#include "Settings/SettingsManager.hpp"
#include "SettingsJson/SettingsJsonSchema.hpp"
#include "State/CentralState.hpp"
#include "State/CompilerTools.hpp"
#include "System/DefinesVersion.hpp"
#include "System/Files.hpp"
#include "Terminal/ColorTheme.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"
#include "Json/JsonKeys.hpp"
#include "Json/JsonValues.hpp"

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

		case QueryOption::Options:
			ret = getOptions();
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

		case QueryOption::ConvertFormats:
			ret = m_centralState.inputs().getConvertFormatPresets();
			break;

		case QueryOption::QueryNames:
			ret = m_centralState.inputs().getCliQueryOptions();
			break;

		case QueryOption::ThemeNames:
			ret = ColorTheme::getPresetNames();
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
			ret = getCurrentLastTarget();
			break;

		case QueryOption::AllRunTargets:
			ret = getAllRunTargets();
			break;

		case QueryOption::AllBuildTargets:
			ret = getAllBuildTargets();
			break;

		case QueryOption::AllToolchains: {
			StringList presets = m_centralState.inputs().getToolchainPresets();
			StringList userToolchains = getUserToolchainList();
			ret = List::combineRemoveDuplicates(std::move(userToolchains), std::move(presets));
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
		return settingsFile.root;
	}
	else if (m_centralState.cache.exists(CacheType::Global))
	{
		const auto& settingsFile = m_centralState.cache.getSettings(SettingsType::Global);
		return settingsFile.root;
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
	const auto& chaletJson = m_centralState.chaletJson().root;

	bool addedDefaults = false;
	if (chaletJson.contains(Keys::DefaultConfigurations))
	{
		const Json& defaultConfigurations = chaletJson[Keys::DefaultConfigurations];
		if (defaultConfigurations.is_array())
		{
			addedDefaults = true;
			for (auto& configJson : defaultConfigurations)
			{
				auto name = json::get<std::string>(configJson);
				if (name.empty() || !List::contains(defaultBuildConfigurations, name))
					continue;

				ret.emplace_back(std::move(name));
			}
		}
	}

	if (!addedDefaults)
	{
		ret = defaultBuildConfigurations;
	}

	if (chaletJson.contains(Keys::Configurations))
	{
		const Json& configurations = chaletJson[Keys::Configurations];
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
			const auto& toolchains = settingsFile[Keys::Toolchains];
			for (auto& [key, _] : toolchains.items())
			{
				ret.emplace_back(key);
			}
		}
	}

	const auto& inputs = m_centralState.inputs();
	const auto& envFile = inputs.envFile();
	if (!envFile.empty() && Files::pathExists(envFile))
	{
		DotEnvFileParser envParser(inputs);
		if (envParser.readVariablesFromFile(envFile))
		{
			auto toolchainNameFromEnv = Environment::getString("CHALET_TOOLCHAIN_NAME");
			if (!toolchainNameFromEnv.empty())
			{
				ret.emplace_back(std::move(toolchainNameFromEnv));
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
				const auto& toolchains = settingsFile[Keys::Toolchains];
				if (toolchains.contains(toolchain))
				{
					const auto& toolchainJson = toolchains[toolchain];
					if (toolchainJson.is_object())
					{
						auto strategy = json::get<std::string>(toolchainJson, Keys::ToolchainBuildStrategy);
						if (!strategy.empty())
							ret.emplace_back(std::move(strategy));
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
				const auto& toolchains = settingsFile[Keys::Toolchains];
				if (toolchains.contains(toolchain))
				{
					const auto& toolchainJson = toolchains[toolchain];
					if (toolchainJson.is_object())
					{
						auto style = json::get<std::string>(toolchainJson, Keys::ToolchainBuildPathStyle);
						if (!style.empty())
							ret.emplace_back(std::move(style));
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
		Values::Auto,
	};
}

/*****************************************************************************/
StringList QueryController::getArchitectures(const std::string& inToolchain) const
{
	StringList ret{ Values::Auto };

	bool handledRest = false;

	// TODO: Link these up with the toolchain presets declared in CommandLineInputs

	if (String::equals("llvm", inToolchain) || String::startsWith("llvm-", inToolchain))
	{
		ret.emplace_back("x86_64");
		ret.emplace_back("i686");
		ret.emplace_back("arm64");
		ret.emplace_back("arm");
#if defined(CHALET_LINUX)
		ret.emplace_back("armhf");
#endif
	}
#if defined(CHALET_MACOS)
	else if (String::equals("apple-llvm", inToolchain))
	{
		ret.emplace_back("universal");
		ret.emplace_back("x86_64");
		ret.emplace_back("arm64");
	}
#endif
	else if (String::equals("gcc", inToolchain))
	{
#if defined(CHALET_WIN32)
		ret.emplace_back("x86_64");
		ret.emplace_back("i686");
#elif defined(CHALET_LINUX)
		ret.emplace_back("x86_64");
		ret.emplace_back("i686");
		ret.emplace_back("arm64");
		ret.emplace_back("arm");
		ret.emplace_back("armhf");
#else
		List::addIfDoesNotExist(ret, m_centralState.inputs().hostArchitecture());
#endif
	}
#if defined(CHALET_WIN32)
	else if (String::startsWith("vs-", inToolchain))
	{
		if (String::equals("arm64", m_centralState.inputs().hostArchitecture()))
		{
			ret.emplace_back("arm64");
			ret.emplace_back("arm64_arm64");
			ret.emplace_back("arm64_x64");
			ret.emplace_back("arm64_x86");
		}
		else
		{
			// TODO: this is hacky. Would rather do this differently
			auto year = ::atoi(inToolchain.substr(3, 4).c_str());
			bool supportsArm = year == 0 || year >= 2019;

			// Default gnu-style
			ret.emplace_back("x86_64");
			ret.emplace_back("i686");
			if (supportsArm)
			{
				ret.emplace_back("arm64");
				ret.emplace_back("arm");
			}
			// x64 host arches
			ret.emplace_back("x64_x64");
			ret.emplace_back("x64_x86");
			if (supportsArm)
			{
				ret.emplace_back("x64_arm64");
				ret.emplace_back("x64_arm");
			}
			//
			ret.emplace_back("x86_x86");
			ret.emplace_back("x86_x64");
			if (supportsArm)
			{
				ret.emplace_back("x86_arm64");
				ret.emplace_back("x86_arm");
			}
			// aliases
			ret.emplace_back("x64");
			ret.emplace_back("x86");
		}
	}
#endif
#if defined(CHALET_ENABLE_INTEL_ICC)
	else if (String::startsWith("intel-classic", inToolchain))
	{
		ret.emplace_back("x86_64");
	#if !defined(CHALET_MACOS)
		ret.emplace_back("i686");
	#endif
	}
#endif
#if defined(CHALET_ENABLE_INTEL_ICX)
	else if (String::startsWith("intel-llvm", inToolchain))
	{
		ret.emplace_back("x86_64");
		ret.emplace_back("i686");
	}
#endif
	else if (String::equals("emscripten", inToolchain))
	{
		ret.emplace_back("wasm32");
	}
	else
	{
		const auto& settingsFile = getSettingsJson();
		if (settingsFile.is_object())
		{
			if (settingsFile.contains(Keys::Toolchains))
			{
				const auto& toolchains = settingsFile[Keys::Toolchains];
				if (toolchains.contains(inToolchain))
				{
					const auto& toolchain = toolchains[inToolchain];
					for (auto& [key, item] : toolchain.items())
					{
						if (item.is_object())
							ret.emplace_back(key);
					}

					handledRest = true;
				}
			}
		}
	}

	if (!handledRest)
	{
		auto currentArch = getCurrentArchitecture();
		if (!currentArch.empty())
		{
			List::addIfDoesNotExist(ret, std::move(currentArch.front()));
		}
	}

	return ret;
}

/*****************************************************************************/
StringList QueryController::getOptions() const
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
			const auto& options = settingsFile[Keys::Options];
			if (options.is_object())
			{
				auto arch = json::get<std::string>(options, Keys::OptionsArchitecture);
				if (!arch.empty())
					ret.emplace_back(std::move(arch));
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
			const auto& options = settingsFile[Keys::Options];
			if (options.is_object())
			{
				auto configuration = json::get<std::string>(options, Keys::OptionsBuildConfiguration);
				if (!configuration.empty())
					ret.emplace_back(std::move(configuration));
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
			const auto& options = settingsFile[Keys::Options];
			if (options.is_object())
			{
				auto toolchain = json::get<std::string>(options, Keys::OptionsToolchain);
				if (!toolchain.empty())
					ret.emplace_back(std::move(toolchain));
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
	StringList ret{ Values::All };

	const auto& chaletJson = m_centralState.chaletJson().root;

	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson[Keys::Targets];
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object())
					continue;

				auto kind = json::get<std::string>(target, Keys::Kind);
				if (kind.empty())
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

	const auto& chaletJson = m_centralState.chaletJson().root;

	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson[Keys::Targets];
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object())
					continue;

				auto kind = json::get<std::string>(target, Keys::Kind);
				if (kind.empty())
					continue;

				if (!String::equals(StringList{ "executable", "script", "process", "cmakeProject" }, kind))
					continue;

				bool isExecutable = true;
				if (String::equals("cmakeProject", kind))
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
StringList QueryController::getCurrentLastTarget() const
{
	StringList ret;

	const auto& settingsFile = getSettingsJson();
	if (settingsFile.is_object())
	{
		if (settingsFile.contains(Keys::Options))
		{
			const auto& options = settingsFile[Keys::Options];
			if (options.is_object())
			{
				auto lastTarget = json::get<std::string>(options, Keys::OptionsLastTarget);
				if (!lastTarget.empty())
					ret.emplace_back(std::move(lastTarget));
			}
		}
	}

	if (!ret.empty())
		return ret;

	const auto& chaletJson = m_centralState.chaletJson().root;

	StringList executableProjects;
	if (chaletJson.is_object())
	{
		if (chaletJson.contains(Keys::Targets))
		{
			const auto& targets = chaletJson[Keys::Targets];
			for (auto& [key, target] : targets.items())
			{
				if (!target.is_object())
					continue;

				auto kind = json::get<std::string>(target, Keys::Kind);
				if (kind.empty())
					continue;

				if (!String::equals(StringList{ "executable", "script", "cmakeProject" }, kind))
					continue;

				bool isExecutable = true;
				if (String::equals("cmakeProject", kind))
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
	output["runTargets"] = getAllRunTargets();
	output["buildTargets"] = getAllBuildTargets();

	auto lastTargetRes = getCurrentLastTarget();
	if (!lastTargetRes.empty())
		output["defaultRunTarget"] = std::move(lastTargetRes.front());

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
	output["allToolchains"] = List::combineRemoveDuplicates(userToolchains, toolchainPresets);
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

	auto lastTargetRes = getCurrentLastTarget();
	if (!lastTargetRes.empty())
	{
		output["lastRunTarget"] = lastTargetRes.front();
		output["lastBuildTarget"] = lastTargetRes.front();
	}

	ret.emplace_back(output.dump());

	return ret;
}

/*****************************************************************************/
Json QueryController::getBuildConfigurationDetails() const
{
	Json ret = Json::object();

	Dictionary<Json> configMap;

	const auto& chaletJson = m_centralState.chaletJson().root;
	if (chaletJson.contains(Keys::Configurations))
	{
		const Json& configurations = chaletJson[Keys::Configurations];
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

	Json schema = ChaletJsonSchema::get(m_centralState.inputs());
	ret.emplace_back(schema.dump());

	return ret;
}

/*****************************************************************************/
StringList QueryController::getSettingsSchema() const
{
	StringList ret;

	Json schema = SettingsJsonSchema::get(m_centralState.inputs());
	ret.emplace_back(schema.dump());

	return ret;
}
}
