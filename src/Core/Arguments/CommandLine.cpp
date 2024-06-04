/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Arguments/CommandLine.hpp"

#include "Core/Arguments/ArgumentParser.hpp"
#include "Core/CommandLineInputs.hpp"

#include "Terminal/Output.hpp"
#include "Utility/String.hpp"
#include "Json/JsonValues.hpp"

namespace chalet
{
/*****************************************************************************/
Unique<CommandLineInputs> CommandLine::read(const i32 argc, const char* argv[], bool& outResult)
{
	Unique<CommandLineInputs> inputs = std::make_unique<CommandLineInputs>();

	ArgumentParser patterns(*inputs);
	outResult = patterns.resolveFromArguments(argc, argv);
	if (!outResult)
		return inputs;

	inputs->setAppPath(patterns.getProgramPath());

	CommandRoute route = patterns.getRoute();
	inputs->setRoute(route);
	if (route.isHelp())
		return inputs;

	std::string buildConfiguration;
	std::string toolchainPreference;
	std::string architecturePreference;
	std::string inputFile;
	std::string settingsFile;
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
				auto& variant = mapped.value();
				switch (id)
				{
					case ArgumentIdentifier::BuildTargetName:
						inputs->setLastTarget(variant.asString());
						break;

					case ArgumentIdentifier::BuildConfiguration:
						buildConfiguration = variant.asString();
						break;

					case ArgumentIdentifier::InputFile:
						inputFile = variant.asString();
						break;

					case ArgumentIdentifier::SettingsFile:
						settingsFile = variant.asString();
						break;

					case ArgumentIdentifier::File:
						settingsFile = variant.asString();
						break;

					case ArgumentIdentifier::RootDirectory:
						rootDirectory = variant.asString();
						break;

					case ArgumentIdentifier::OutputDirectory:
						outputDirectory = variant.asString();
						break;

					case ArgumentIdentifier::ExternalDirectory:
						externalDirectory = variant.asString();
						break;

					case ArgumentIdentifier::DistributionDirectory:
						distributionDirectory = variant.asString();
						break;

					case ArgumentIdentifier::Toolchain:
						toolchainPreference = variant.asString();
						break;

					case ArgumentIdentifier::SigningIdentity:
						inputs->setSigningIdentity(variant.asString());
						break;

					case ArgumentIdentifier::OsTargetName:
						inputs->setOsTargetName(variant.asString());
						break;

					case ArgumentIdentifier::OsTargetVersion:
						inputs->setOsTargetVersion(variant.asString());
						break;

					case ArgumentIdentifier::ExportKind:
						inputs->setExportKind(variant.asString());
						break;

					case ArgumentIdentifier::EnvFile:
						envFile = variant.asString();
						break;

					case ArgumentIdentifier::TargetArchitecture:
						architecturePreference = variant.asString();
						break;

					case ArgumentIdentifier::BuildStrategy:
						inputs->setBuildStrategyPreference(variant.asString());
						break;

					case ArgumentIdentifier::BuildPathStyle:
						inputs->setBuildPathStylePreference(variant.asString());
						break;

					case ArgumentIdentifier::InitPath:
						inputs->setInitPath(variant.asString());
						break;

					case ArgumentIdentifier::InitTemplate:
						inputs->setInitTemplate(variant.asString());
						break;

					case ArgumentIdentifier::SettingsKey:
						inputs->setSettingsKey(variant.asString());
						break;

					case ArgumentIdentifier::ValidateSchemaFile:
						settingsFile = variant.asString();
						break;

					case ArgumentIdentifier::QueryType:
						inputs->setQueryOption(variant.asString());
						break;

					case ArgumentIdentifier::ConvertFormat:
						inputs->setSettingsKey(variant.asString());
						break;

					case ArgumentIdentifier::ExportBuildConfigurations:
						inputs->setExportBuildConfigurations(variant.asString());
						break;

					case ArgumentIdentifier::ExportArchitectures:
						inputs->setExportArchitectures(variant.asString());
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::StringList: {
				auto& variant = mapped.value();
				switch (id)
				{
					case ArgumentIdentifier::ValidateFilesRemainingArgs:
						inputs->setRunArguments(variant.asStringList());
						break;

					case ArgumentIdentifier::RunTargetArguments:
						inputs->setRunArguments(variant.asStringList());
						break;

					case ArgumentIdentifier::QueryDataRemainingArgs:
						inputs->setQueryData(variant.asStringList());
						break;

					case ArgumentIdentifier::SettingsValueRemainingArgs: {
						auto list = variant.asStringList();
						if (!list.empty())
						{
							inputs->setSettingsValue(std::move(list.front()));
						}
						break;
					}

					case ArgumentIdentifier::SettingsKeysRemainingArgs:
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::OptionalInteger: {
				auto rawValue = mapped.value().asOptionalInt();
				if (!rawValue.has_value())
					break;

				i32 value = *rawValue;

				if (id == ArgumentIdentifier::MaxJobs)
				{
					inputs->setMaxJobs(static_cast<u32>(value));
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

					case ArgumentIdentifier::CompilerCache:
						inputs->setCompilerCache(value);
						break;

					case ArgumentIdentifier::GenerateCompileCommands:
						inputs->setGenerateCompileCommands(value);
						break;

					case ArgumentIdentifier::OnlyRequired:
						inputs->setOnlyRequired(value);
						break;

					case ArgumentIdentifier::SaveSchema:
						inputs->setSaveSchemaToFile(value);
						break;

					case ArgumentIdentifier::SaveUserToolchainGlobally:
						inputs->setSaveUserToolchainGlobally(value);
						break;

					case ArgumentIdentifier::Quieter:
						Output::setQuietNonBuild(value);
						break;

					default: break;
				}

				break;
			}

			case Variant::Kind::Boolean: {
				bool value = mapped.value().asBool();
				switch (id)
				{
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

					case ArgumentIdentifier::ExportOpen: {
						inputs->setOpenAfterExport(value);
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

	inputs->setSettingsFile(std::move(settingsFile));

	inputs->setBuildConfiguration(std::move(buildConfiguration));

	if (!toolchainPreference.empty() && architecturePreference.empty())
	{
		inputs->setArchitectureRaw(Values::Auto);
		inputs->setToolchainPreference(std::move(toolchainPreference));
	}
	else
	{
		inputs->setArchitectureRaw(std::move(architecturePreference));

		// must do at the end (after arch & toolchain have been parsed)
		inputs->setToolchainPreference(std::move(toolchainPreference));
	}

	if (route.isQuery())
	{
		// Output::setQuietNonBuild(true);
		inputs->setCommandList(patterns.getRouteList());
	}

	return inputs;
}
}
