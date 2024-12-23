/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Check/BuildFileChecker.hpp"

#include "Bundler/AppBundler.hpp"
#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CentralState.hpp"
#include "State/PackageManager.hpp"
#include "State/Target/IBuildTarget.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Shell.hpp"
#include "Yaml/YamlFile.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
namespace
{
constexpr const char kCondition[] = "condition";
}

/*****************************************************************************/
BuildFileChecker::BuildFileChecker(BuildState& inState) :
	m_state(inState),
	m_parser(m_state)
{
}

/*****************************************************************************/
bool BuildFileChecker::run()
{
	// auto& inputs = m_centralState.inputs();
	auto& theme = Output::theme();

	Output::printSeparator();

	{
		auto& buildFile = m_state.getCentralState().chaletJson();
		Json checked = getExpandedBuildFile();

		Output::printInfo(buildFile.filename());
		Output::lineBreak();

		std::string contents;
		if (String::endsWith(".yaml", buildFile.filename()))
			contents = YamlFile::asString(checked);
		else
			contents = checked.dump(3, ' ');

		const auto& buildColor = Output::getAnsiStyle(theme.build);
		const auto& reset = Output::getAnsiStyle(theme.reset);

		std::cout.write(buildColor.data(), buildColor.size());
		std::cout.write(contents.data(), contents.size());
		std::cout.write(reset.data(), reset.size());
		std::cout.put('\n');
		std::cout.flush();
	}

	Output::lineBreak();
	Output::printSeparator();

	{
		const auto& flair = Output::getAnsiStyle(theme.flair);
		const auto& buildColor = Output::getAnsiStyle(theme.build);
		const auto& reset = Output::getAnsiStyle(theme.reset);

		std::string output{ "Subtitutions\n\n" };

		BuildState::VariableOptions options;
		options.validateExternals = false;
		auto getOutputLineFor = [this, &options, &flair, &buildColor, &reset](const char* key) {
			auto value = fmt::format("${{{}}}", key);
			std::string dots(27 - value.size(), '.');
			m_state.replaceVariablesInString(value, static_cast<const IBuildTarget*>(nullptr), options);
			if (value.empty())
				return value;

			return fmt::format("${{{}}}{} {} {}{}{}\n", key, flair, dots, buildColor, value, reset);
		};

		auto getSimulatedOutputLine = [&flair, &buildColor, &reset](const std::string& key, std::string value) {
			std::string dots(27 - (key.size() + 3), '.');
			return fmt::format("${{{}}}{} {} {}{}{}\n", key, flair, dots, buildColor, value, reset);
		};

		output += getOutputLineFor("meta:workspaceName");
		output += getOutputLineFor("meta:workspaceVersion");
		output += getOutputLineFor("cwd");
		output += getOutputLineFor("architecture");
		output += getOutputLineFor("targetTriple");
		output += getOutputLineFor("configuration");
		output += getOutputLineFor("home");
		output += getOutputLineFor("maxJobs");
		output += getOutputLineFor("outputDir");
		output += getOutputLineFor("buildDir");
		output += getSimulatedOutputLine("name", "foo");
		output += getOutputLineFor("meta:name");
		output += getOutputLineFor("meta:version");
		output += getOutputLineFor("external:foo");
		output += getOutputLineFor("externalBuild:foo");
		output += getOutputLineFor("so:foo");
		output += getOutputLineFor("ar:foo");
		output += getOutputLineFor("exe:foo");

		bool comspec = false;
		auto shell = getOutputLineFor("env:SHELL");
#if defined(CHALET_WIN32)
		if (shell.empty() && Shell::isMicrosoftTerminalOrWindowsBash())
		{
			shell = getOutputLineFor("env:COMSPEC");
			comspec = true;
		}
#endif

		output += std::move(shell);

#if defined(CHALET_WIN32)
		if (comspec)
			output += getOutputLineFor("defined:COMSPEC");
		else
#endif
			output += getOutputLineFor("defined:SHELL");

		{
		}

		std::cout.write(output.data(), output.size());
		std::cout.flush();
	}

	Output::lineBreak();
	Output::printSeparator();

	return true;
}

/*****************************************************************************/
Json BuildFileChecker::getExpandedBuildFile()
{
	auto& buildFile = m_state.getCentralState().chaletJson();

	Json checked;
	if (!checkNode(buildFile.root, checked))
		return false;

	const std::string kExternalDependencies{ "externalDependencies" };
	const std::string kPackage{ "package" };
	const std::string kTargets{ "targets" };
	const std::string kDistribution{ "distribution" };

	if (checked.contains(kExternalDependencies))
	{
		auto& externalDependenciesJson = checked[kExternalDependencies];
		for (auto& target : m_state.externalDependencies)
		{
			const auto& name = target->name();
			if (externalDependenciesJson.contains(name))
				checkNodeWithExternalDependency(externalDependenciesJson[name], target.get());
		}
	}

	if (checked.contains(kPackage))
	{
		auto& packageJson = checked[kPackage];
		for (const auto& [key, value] : packageJson.items())
		{
			auto package = m_state.packages.getSourcePackage(key);
			if (package != nullptr)
				checkNodeWithTargetPtr(packageJson, package);
		}
	}

	if (checked.contains(kTargets))
	{
		auto& targetsJson = checked[kTargets];
		for (auto& target : m_state.targets)
		{
			const auto& name = target->name();
			if (targetsJson.contains(name))
				checkNodeWithTargetPtr(targetsJson[name], target.get());
		}
	}

	if (checked.contains(kDistribution))
	{
		AppBundler bundler(m_state);

		auto& distributionJson = checked[kDistribution];
		auto newJson = Json::object();
		for (auto& target : m_state.distribution)
		{
			Json newNode;
			const auto& name = target->name();
			if (distributionJson.contains(name))
			{
				newNode = std::move(distributionJson[name]);

				std::string tname = name;
				if (!bundler.isTargetNameValid(*target, tname))
					return false;

				newJson[tname] = std::move(newNode);
				checkNodeWithTargetPtr(newJson[tname], target.get());
			}
		}
		checked[kDistribution] = std::move(newJson);
	}

	return checked;
}

/*****************************************************************************/
bool BuildFileChecker::checkNode(const Json& inNode, Json& outJson, const std::string& inLastKey)
{
	if (inNode.is_object())
	{
		if (!outJson.is_object())
			outJson = Json::object();

		if (inNode.contains(kCondition))
		{
			auto& condition = inNode[kCondition];
			if (condition.is_string())
			{
				auto result = m_parser.conditionIsValid(inLastKey, condition.get<std::string>());
				if (!result.has_value()) // syntax error
					return false;

				bool conditionValid = *result;
				outJson[kCondition] = conditionValid;
				if (!conditionValid)
					return false;
			}
		}

		for (const auto& [keyRaw, value] : inNode.items())
		{
			Json newNode;
			if (!checkNode(value, newNode, keyRaw))
				continue;

			std::string key = keyRaw;
			auto firstSquareBracket = key.find('[');
			if (firstSquareBracket != std::string::npos)
			{
				JsonNodeReadStatus status = JsonNodeReadStatus::Unread;
				auto subkey = key.substr(0, firstSquareBracket);
				if (value.is_array())
				{
					StringList val;
					if (!m_parser.valueMatchesSearchKeyPattern(val, value, key, subkey.c_str(), status))
						continue;

					if (outJson.contains(subkey) && outJson[subkey].is_array())
					{
						for (auto& v : val)
						{
							outJson[subkey].push_back(v);
						}
					}
					else
					{
						outJson[subkey] = val;
					}
				}

				key = subkey;
			}

			if (!outJson.contains(key))
			{
				outJson[key] = std::move(newNode);
			}
		}
	}
	else if (inNode.is_array())
	{
		if (!outJson.is_array())
			outJson = Json::array();

		for (const auto& value : inNode)
		{
			auto& node = outJson.emplace_back();
			if (!checkNode(value, node, inLastKey))
				return false;
		}
	}
	else
	{
		outJson = inNode;
	}

	return true;
}

/*****************************************************************************/
bool BuildFileChecker::checkNodeWithExternalDependency(Json& node, const IExternalDependency* inTarget)
{
	if (node.is_object())
	{
		for (const auto& [keyRaw, value] : node.items())
		{
			checkNodeWithExternalDependency(value, inTarget);
		}
	}
	else if (node.is_array())
	{
		for (auto& value : node)
		{
			checkNodeWithExternalDependency(value, inTarget);
		}
	}
	else if (node.is_string())
	{
		auto value = node.get<std::string>();
		m_state.getCentralState().replaceVariablesInString(value, inTarget);
		node = value;
	}

	return true;
}
}
