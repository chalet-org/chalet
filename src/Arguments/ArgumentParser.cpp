/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/ArgumentParser.hpp"

#include "Arguments/ArgumentPatterns.hpp"
#include "Arguments/CLIParser.hpp"
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
bool ArgumentParser::run(const int argc, const char* argv[])
{
	if (argc < 1)
		return false;

	ArgumentPatterns patterns(m_inputs);
	bool result = patterns.resolveFromArguments(argc, argv);
	if (!result)
		return false;

	m_inputs.setAppPath(patterns.getProgramPath());

	Route route = patterns.route();
	m_inputs.setRoute(route);
	if (route == Route::Help)
		return true;

	if (patterns.arguments().empty())
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

	for (auto& mapped : patterns.arguments())
	{
		const auto id = mapped.id();
		// LOG(mapped.value(), "-----", mapped.key(), mapped.keyLong());
		auto kind = mapped.value().kind();
		switch (kind)
		{
			case Variant::Kind::String: {
				auto value = mapped.value().asString();
				if (value.empty())
					continue;

				switch (id)
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

					case ArgumentIdentifier::InitTemplate:
						m_inputs.setInitTemplate(std::move(value));
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

					case ArgumentIdentifier::RunTargetArguments:
						m_inputs.setRunArguments(std::move(value));
						break;

					case ArgumentIdentifier::SettingsKeysRemainingArgs:
						break;

					case ArgumentIdentifier::QueryDataRemainingArgs:
						m_inputs.setQueryData(String::split(value, ' '));
						break;

					default: break;
				}

				break;
			}

				/*case Variant::Kind::StringList: {
				if (id == ArgumentIdentifier::RunTargetArguments)
				{
					auto runArgs = String::join(mapped.value().asStringList());
					m_inputs.setRunArguments(std::move(runArgs));
				}
				else if (id == ArgumentIdentifier::SettingsKeysRemainingArgs)
				{
					// ignore
				}
				else if (id == ArgumentIdentifier::QueryDataRemainingArgs)
				{
					m_inputs.setQueryData(mapped.value().asStringList());
				}
				break;
			}*/

			case Variant::Kind::OptionalInteger: {
				auto rawValue = mapped.value().asOptionalInt();
				if (!rawValue.has_value())
					break;

				int value = *rawValue;

				if (id == ArgumentIdentifier::MaxJobs)
				{
					m_inputs.setMaxJobs(static_cast<uint>(value));
				}
				break;
			}

			case Variant::Kind::OptionalBoolean: {
				auto rawValue = mapped.value().asOptionalBool();
				if (!rawValue.has_value())
					break;

				bool value = *rawValue;

				switch (id)
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

					case ArgumentIdentifier::LaunchProfiler:
						m_inputs.setLaunchProfiler(value);
						break;

					case ArgumentIdentifier::KeepGoing:
						m_inputs.setKeepGoing(value);
						break;

					case ArgumentIdentifier::GenerateCompileCommands:
						m_inputs.setGenerateCompileCommands(value);
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::Boolean: {
				bool value = mapped.value().asBool();
				switch (id)
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

	if (!toolchainPreference.empty() && architecturePreference.empty())
	{
		m_inputs.setArchitectureRaw("auto");
		m_inputs.setToolchainPreference(std::move(toolchainPreference));
	}
	else
	{
		m_inputs.setArchitectureRaw(std::move(architecturePreference));

		// must do at the end (after arch & toolchain have been parsed)
		m_inputs.setToolchainPreference(std::move(toolchainPreference));
	}

	if (route == Route::Query)
	{
		// Output::setQuietNonBuild(true);
		m_inputs.setCommandList(patterns.getRouteList());
	}

	return true;
}
}
