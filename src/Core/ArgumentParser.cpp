/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentParser.hpp"

#include "Core/ArgumentPatterns.hpp"
// #include "Core/ArgumentMap.hpp"
#include "Core/CommandLineInputs.hpp"

#include "Router/Route.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ArgumentParser::ArgumentParser(CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool ArgumentParser::run(const int argc, const char* const argv[])
{
	if (argc < 1)
		return false;

	StringList arguments = parseRawArguments(argc, argv);

	ArgumentPatterns patterns;
	bool result = patterns.parse(arguments);
	if (!result)
		return false;

	Route command = patterns.route();
	m_inputs.setCommand(command);
	if (command == Route::Help)
		return true;

	if (patterns.arguments().size() == 0)
		return false;

	m_inputs.setAppPath(arguments.front());

	std::string buildConfiguration;
	std::string toolchainPreference;
	std::string architecturePreference;
	std::string inputFile;
	std::string settingsFile;
	std::string file;
	std::string rootDirectory;
	std::string outputDirectory;
	std::string externalDirectory;
	std::string distributionDirectory;
	std::string envFile;

	for (auto& [key, rawValue] : patterns.arguments())
	{
		auto kind = rawValue.kind();
		switch (kind)
		{
			case Variant::Kind::String: {
				auto value = rawValue.asString();
				if (value.empty())
					continue;

				if (key == patterns.argRunProject())
				{
					m_inputs.setRunProject(std::move(value));
				}
				else if (String::equals({ "-c", "--configuration" }, key))
				{
					buildConfiguration = std::move(value);
				}
				else if (String::equals({ "-i", "--input-file" }, key))
				{
					inputFile = std::move(value);
				}
				else if (String::equals({ "-s", "--settings-file" }, key))
				{
					settingsFile = std::move(value);
				}
				else if (String::equals({ "-f", "--file" }, key))
				{
					file = std::move(value);
				}
				else if (String::equals({ "-r", "--root-dir" }, key))
				{
					rootDirectory = std::move(value);
				}
				else if (String::equals({ "-o", "--output-dir" }, key))
				{
					outputDirectory = std::move(value);
				}
				else if (String::equals({ "-x", "--external-dir" }, key))
				{
					externalDirectory = std::move(value);
				}
				else if (String::equals({ "-d", "--distribution-dir" }, key))
				{
					distributionDirectory = std::move(value);
				}
				else if (String::equals({ "-t", "--toolchain" }, key))
				{
					toolchainPreference = std::move(value);
				}
				/*else if (String::equals({ "-p", "--project-gen" }, key))
				{
					m_inputs.setGenerator(std::move(value));
				}*/
				else if (String::equals({ "-e", "--env-file" }, key))
				{
					envFile = std::move(value);
				}
				else if (String::equals({ "-a", "--arch" }, key))
				{
					architecturePreference = std::move(value);
				}
				else if (key == patterns.argInitPath())
				{
					m_inputs.setInitPath(std::move(value));
				}
				else if (key == patterns.argConfigKey())
				{
					m_inputs.setSettingsKey(std::move(value));
				}
				else if (key == patterns.argConfigValue())
				{
					m_inputs.setSettingsValue(std::move(value));
				}
				break;
			}

			case Variant::Kind::StringList: {
				if (key == patterns.argRunArguments())
				{
					auto runArgs = String::join(rawValue.asStringList());
					m_inputs.setRunOptions(std::move(runArgs));
				}
				break;
			}

			case Variant::Kind::Boolean: {
				bool value = rawValue.asBool();
				if (String::equals("--save-schema", key))
				{
					m_inputs.setSaveSchemaToFile(value);
				}
				else if (String::equals("--quieter", key))
				{
					Output::setQuietNonBuild(value);
				}
				else if (String::equals({ "-l", "--local" }, key))
				{
					if (value)
						m_inputs.setSettingsType(SettingsType::Local);
				}
				else if (String::equals({ "-g", "--global" }, key))
				{
					if (value)
						m_inputs.setSettingsType(SettingsType::Global);
				}
				break;
			}
			default:
				break;
		}
	}

	// root must be first
	m_inputs.setRootDirectory(std::move(rootDirectory));

	//
	m_inputs.setExternalDirectory(std::move(externalDirectory));
	m_inputs.setOutputDirectory(std::move(outputDirectory));
	m_inputs.setDistributionDirectory(std::move(distributionDirectory));
	m_inputs.setInputFile(std::move(inputFile));
	m_inputs.setEnvFile(std::move(envFile));

	if (!file.empty())
		m_inputs.setSettingsFile(std::move(file));
	else
		m_inputs.setSettingsFile(std::move(settingsFile));

	m_inputs.setBuildConfiguration(std::move(buildConfiguration));
	m_inputs.setArchitectureRaw(std::move(architecturePreference));

	// must do at the end (after arch & toolchain have been parsed)
	m_inputs.setToolchainPreference(std::move(toolchainPreference));

	return true;
}

/*****************************************************************************/
StringList ArgumentParser::parseRawArguments(const int argc, const char* const argv[])
{
	StringList ret;

	for (int i = 0; i < argc; ++i)
	{
		std::string arg(argv[i] ? argv[i] : "");
		if (i == 0)
		{
			ret.emplace_back(std::move(arg));
			continue;
		}

		if (String::startsWith('"', arg))
		{
			arg = arg.substr(1);

			if (arg.back() == '"')
				arg.pop_back();
		}

		if (String::startsWith('\'', arg))
		{
			arg = arg.substr(1);

			if (arg.back() == '\'')
				arg.pop_back();
		}

		if (String::startsWith("--", arg) && String::contains('=', arg))
		{
			auto list = String::split(arg, '=');
			for (auto& it : list)
			{
				ret.emplace_back(std::move(it));
			}
		}
		else
		{
			ret.emplace_back(std::move(arg));
		}
	}

	return ret;
}
}
