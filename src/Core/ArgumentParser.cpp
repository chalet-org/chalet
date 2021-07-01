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

	StringList arguments;
	for (int i = 0; i < argc; ++i)
	{
		std::string arg(argv[i] ? argv[i] : "");
		if (i == 0)
		{
			arguments.emplace_back(std::move(arg));
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
				arguments.emplace_back(std::move(it));
			}
		}
		else
		{
			arguments.emplace_back(std::move(arg));
		}
	}

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

	std::string toolchainPreference;
	std::string inputFile;
	std::string settingsFile;
	std::string file;
	std::string rootDirectory;
	std::string outputDirectory;
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

				if (key == patterns.argBuildConfiguration())
				{
					m_inputs.setBuildFromCommandLine(std::move(value));
				}
				else if (key == patterns.argRunProject())
				{
					m_inputs.setRunProject(std::move(value));
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
					m_inputs.setArchRaw(value);
					if (String::contains(',', value))
					{
						auto firstComma = value.find(',');
						auto arch = value.substr(0, firstComma);
						auto options = String::split(value.substr(firstComma + 1), ',');
						m_inputs.setTargetArchitecture(std::move(arch));
						m_inputs.setArchOptions(std::move(options));
					}
					else
					{
						m_inputs.setTargetArchitecture(std::move(value));
					}
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
	m_inputs.setOutputDirectory(std::move(outputDirectory));
	m_inputs.setInputFile(std::move(inputFile));
	if (!file.empty())
		m_inputs.setSettingsFile(std::move(file));
	else
		m_inputs.setSettingsFile(std::move(settingsFile));
	m_inputs.setEnvFile(std::move(envFile));

	// must do at the end (after arch & toolchain have been parsed)
	m_inputs.setToolchainPreference(std::move(toolchainPreference));

	return true;
}
}
