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
	m_inputs.setAppPath(arguments.front());

	ArgumentPatterns patterns(m_inputs);
	bool result = patterns.resolveFromArguments(arguments);
	if (!result)
		return false;

	Route route = patterns.route();
	m_inputs.setCommand(route);
	if (route == Route::Help)
		return true;

	if (patterns.arguments().size() == 0)
		return false;

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

	for (auto& [key, arg] : patterns.arguments())
	{
		auto kind = arg.value.kind();
		switch (kind)
		{
			case Variant::Kind::String: {
				auto value = arg.value.asString();
				if (value.empty())
					continue;

				switch (arg.id)
				{
					case ArgumentIdentifier::RunTargetName:
						m_inputs.setRunTarget(std::move(value));
						break;

					case ArgumentIdentifier::BuildConfiguration:
						buildConfiguration = std::move(value);
						break;

					case ArgumentIdentifier::InputFile:
						inputFile = std::move(value);
						break;

					case ArgumentIdentifier::SettingsFile:
						settingsFile = std::move(value);
						break;

					case ArgumentIdentifier::File:
						file = std::move(value);
						break;

					case ArgumentIdentifier::RootDirectory:
						rootDirectory = std::move(value);
						break;

					case ArgumentIdentifier::OutputDirectory:
						outputDirectory = std::move(value);
						break;

					case ArgumentIdentifier::ExternalDirectory:
						externalDirectory = std::move(value);
						break;

					case ArgumentIdentifier::DistributionDirectory:
						distributionDirectory = std::move(value);
						break;

					case ArgumentIdentifier::Toolchain:
						toolchainPreference = std::move(value);
						break;

						// case ProjectGen:
						// 	m_inputs.setGenerator(std::move(value));
						// 	break;

					case ArgumentIdentifier::EnvFile:
						envFile = std::move(value);
						break;

					case ArgumentIdentifier::TargetArchitecture:
						architecturePreference = std::move(value);
						break;

					case ArgumentIdentifier::InitPath:
						m_inputs.setInitPath(std::move(value));
						break;

					case ArgumentIdentifier::SettingsKey:
						m_inputs.setSettingsKey(std::move(value));
						break;

					case ArgumentIdentifier::SettingsValue:
						m_inputs.setSettingsValue(std::move(value));
						break;

					case ArgumentIdentifier::QueryType:
						m_inputs.setQueryOption(std::move(value));
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::StringList: {
				if (arg.id == ArgumentIdentifier::RunTargetArguments)
				{
					auto runArgs = String::join(arg.value.asStringList());
					m_inputs.setRunOptions(std::move(runArgs));
				}
				else if (arg.id == ArgumentIdentifier::SettingsKeysRemainingArgs)
				{
					// ignore
				}
				break;
			}

			case Variant::Kind::OptionalInteger: {
				auto rawValue = arg.value.asOptionalInt();
				if (!rawValue.has_value())
					break;

				int value = *rawValue;

				if (arg.id == ArgumentIdentifier::MaxJobs)
				{
					m_inputs.setMaxJobs(static_cast<uint>(value));
				}
				break;
			}

			case Variant::Kind::OptionalBoolean: {
				auto rawValue = arg.value.asOptionalBool();
				if (!rawValue.has_value())
					break;

				bool value = *rawValue;

				switch (arg.id)
				{
					case ArgumentIdentifier::DumpAssembly:
						m_inputs.setDumpAssembly(value);
						break;

					case ArgumentIdentifier::ShowCommands:
						m_inputs.setShowCommands(value);
						break;

					case ArgumentIdentifier::Benchmark:
						m_inputs.setBenchmark(value);
						break;

					case ArgumentIdentifier::GenerateCompileCommands:
						m_inputs.setGenerateCompileCommands(value);
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::Boolean: {
				bool value = arg.value.asBool();
				switch (arg.id)
				{
					case ArgumentIdentifier::SaveSchema:
						m_inputs.setSaveSchemaToFile(value);
						break;

					case ArgumentIdentifier::Quieter:
						Output::setQuietNonBuild(value);
						break;

					case ArgumentIdentifier::LocalSettings: {
						if (value)
							m_inputs.setSettingsType(SettingsType::Local);
						break;
					}

					case ArgumentIdentifier::GlobalSettings: {
						if (value)
							m_inputs.setSettingsType(SettingsType::Global);
						break;
					}

					default: break;
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

	if (route == Route::Query)
	{
		// Output::setQuietNonBuild(true);
		m_inputs.setCommandList(patterns.getRouteList());
	}

	return true;
}

/*****************************************************************************/
StringList ArgumentParser::parseRawArguments(const int argc, const char* const argv[])
{
	StringList ret;

	StringList implicitTrueArgs{
		"--dump-assembly",
		"--generate-compile-commands",
		"--show-commands",
		"--benchmark"
	};

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
			String::replaceAll(arg, "=true", "=1");
			String::replaceAll(arg, "=false", "=0");

			auto list = String::split(arg, '=');
			for (auto& it : list)
			{
				ret.emplace_back(std::move(it));
			}
		}
		else
		{
			ret.emplace_back(std::move(arg));

			if (String::equals(implicitTrueArgs, ret.back()))
			{
				// This is a hack so that one can write --dump-assembly without an =# or 2nd arg
				// argparse has issues figuring out what you want otherwise
				ret.emplace_back("1");
			}
		}
	}

	return ret;
}
}
