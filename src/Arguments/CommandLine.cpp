/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/CommandLine.hpp"

#include "Arguments/ArgumentParser.hpp"
#include "Core/CommandLineInputs.hpp"

#include "Router/Route.hpp"
#include "Terminal/Output.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
Unique<CommandLineInputs> CommandLine::read(const int argc, const char* argv[], bool& outResult)
{
	Unique<CommandLineInputs> inputs = std::make_unique<CommandLineInputs>();

	ArgumentParser patterns(*inputs);
	outResult = patterns.resolveFromArguments(argc, argv);
	if (!outResult)
		return inputs;

	inputs->setAppPath(patterns.getProgramPath());

	Route route = patterns.route();
	inputs->setRoute(route);
	if (route == Route::Help)
		return inputs;

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
						inputs->setRunTarget(std::move(value));
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

					case ArgumentIdentifier::ExportKind:
						inputs->setExportKind(std::move(value));
						break;

					case ArgumentIdentifier::EnvFile:
						envFile = std::move(value);
						break;

					case ArgumentIdentifier::TargetArchitecture:
						architecturePreference = std::move(value);
						break;

					case ArgumentIdentifier::InitPath:
						inputs->setInitPath(std::move(value));
						break;

					case ArgumentIdentifier::InitTemplate:
						inputs->setInitTemplate(std::move(value));
						break;

					case ArgumentIdentifier::SettingsKey:
						inputs->setSettingsKey(std::move(value));
						break;

					case ArgumentIdentifier::SettingsValue:
						inputs->setSettingsValue(std::move(value));
						break;

					case ArgumentIdentifier::QueryType:
						inputs->setQueryOption(std::move(value));
						break;

					case ArgumentIdentifier::RunTargetArguments:
						inputs->setRunArguments(std::move(value));
						break;

					case ArgumentIdentifier::SettingsKeysRemainingArgs:
						break;

					case ArgumentIdentifier::QueryDataRemainingArgs:
						inputs->setQueryData(String::split(value, ' '));
						break;

					default: break;
				}

				break;
			}

				/*case Variant::Kind::StringList: {
				if (id == ArgumentIdentifier::RunTargetArguments)
				{
					auto runArgs = String::join(mapped.value().asStringList());
					inputs->setRunArguments(std::move(runArgs));
				}
				else if (id == ArgumentIdentifier::SettingsKeysRemainingArgs)
				{
					// ignore
				}
				else if (id == ArgumentIdentifier::QueryDataRemainingArgs)
				{
					inputs->setQueryData(mapped.value().asStringList());
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
					inputs->setMaxJobs(static_cast<uint>(value));
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
						inputs->setDumpAssembly(value);
						break;

					case ArgumentIdentifier::ShowCommands:
						inputs->setShowCommands(value);
						break;

					case ArgumentIdentifier::Benchmark:
						inputs->setBenchmark(value);
						break;

					case ArgumentIdentifier::LaunchProfiler:
						inputs->setLaunchProfiler(value);
						break;

					case ArgumentIdentifier::KeepGoing:
						inputs->setKeepGoing(value);
						break;

					case ArgumentIdentifier::GenerateCompileCommands:
						inputs->setGenerateCompileCommands(value);
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
						inputs->setSaveSchemaToFile(value);
						break;

					case ArgumentIdentifier::Quieter:
						Output::setQuietNonBuild(value);
						break;

					case ArgumentIdentifier::LocalSettings: {
						if (value)
							inputs->setSettingsType(SettingsType::Local);
						break;
					}

					case ArgumentIdentifier::GlobalSettings: {
						if (value)
							inputs->setSettingsType(SettingsType::Global);
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
	inputs->setRootDirectory(std::move(rootDirectory));

	//
	inputs->setExternalDirectory(std::move(externalDirectory));
	inputs->setOutputDirectory(std::move(outputDirectory));
	inputs->setDistributionDirectory(std::move(distributionDirectory));
	inputs->setInputFile(std::move(inputFile));
	inputs->setEnvFile(std::move(envFile));

	if (!file.empty())
		inputs->setSettingsFile(std::move(file));
	else
		inputs->setSettingsFile(std::move(settingsFile));

	inputs->setBuildConfiguration(std::move(buildConfiguration));

	if (!toolchainPreference.empty() && architecturePreference.empty())
	{
		inputs->setArchitectureRaw("auto");
		inputs->setToolchainPreference(std::move(toolchainPreference));
	}
	else
	{
		inputs->setArchitectureRaw(std::move(architecturePreference));

		// must do at the end (after arch & toolchain have been parsed)
		inputs->setToolchainPreference(std::move(toolchainPreference));
	}

	if (route == Route::Query)
	{
		// Output::setQuietNonBuild(true);
		inputs->setCommandList(patterns.getRouteList());
	}

	return inputs;
}
}
