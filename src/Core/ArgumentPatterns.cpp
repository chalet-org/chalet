/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentPatterns.hpp"

#include <thread>

#include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
#define CH_STR(x) static constexpr const char x[]

namespace Arg
{
CH_STR(RunTarget) = "[<run-target>]";
CH_STR(RemainingArguments) = "[ARG...]";
// CH_STR(InitName) = "<name>";
CH_STR(InitPath) = "<path>";
CH_STR(SettingsKey) = "<key>";
CH_STR(SettingsKeyQuery) = "<query>";
CH_STR(SettingsValue) = "<value>";
CH_STR(QueryType) = "<type>";
// CH_STR(QueryData) = "<data>";
}

#undef CH_STR

/*****************************************************************************/
MappedArgument::MappedArgument(ArgumentIdentifier inId, Variant inValue) :
	id(inId),
	value(std::move(inValue))
{
}

/*****************************************************************************/
ArgumentPatterns::ArgumentPatterns(const CommandLineInputs& inInputs) :
	m_inputs(inInputs),
	m_subCommands({
		{ Route::BuildRun, &ArgumentPatterns::populateBuildRunArguments },
		{ Route::Run, &ArgumentPatterns::populateRunArguments },
		{ Route::Build, &ArgumentPatterns::populateBuildArguments },
		{ Route::Rebuild, &ArgumentPatterns::populateRebuildArguments },
		{ Route::Clean, &ArgumentPatterns::populateCleanArguments },
		{ Route::Bundle, &ArgumentPatterns::populateBundleArguments },
		{ Route::Configure, &ArgumentPatterns::populateConfigureArguments },
		{ Route::Init, &ArgumentPatterns::populateInitArguments },
		{ Route::SettingsGet, &ArgumentPatterns::populateSettingsGetArguments },
		{ Route::SettingsGetKeys, &ArgumentPatterns::populateSettingsGetKeysArguments },
		{ Route::SettingsSet, &ArgumentPatterns::populateSettingsSetArguments },
		{ Route::SettingsUnset, &ArgumentPatterns::populateSettingsUnsetArguments },
		{ Route::Query, &ArgumentPatterns::populateQueryArguments },
		{ Route::ColorTest, &ArgumentPatterns::populateColorTestArguments },
	}),
	// clang-format off
	m_optionPairsCache({
		"-i", "--input-file",
		"-s", "--settings-file",
		"-f", "--file",
		"-r", "--root-dir",
		"-o", "--output-dir",
		"-x", "--external-dir",
		"-d", "--distribution-dir",
		// "-p", "--project-gen",
		"-t", "--toolchain",
		"-j", "--max-jobs",
		"-e", "--env-file",
		"-a", "--arch",
		"-c", "--configuration",
		//
		"--template",
	}),
	// clang-format on
	m_routeMap({
		{ "buildrun", Route::BuildRun },
		{ "run", Route::Run },
		{ "build", Route::Build },
		{ "rebuild", Route::Rebuild },
		{ "clean", Route::Clean },
		{ "bundle", Route::Bundle },
		{ "configure", Route::Configure },
		{ "init", Route::Init },
		{ "get", Route::SettingsGet },
		{ "getkeys", Route::SettingsGetKeys },
		{ "set", Route::SettingsSet },
		{ "unset", Route::SettingsUnset },
		{ "query", Route::Query },
		{ "colortest", Route::ColorTest },
	})
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(Route::Debug, &ArgumentPatterns::populateDebugArguments);
	m_routeMap.emplace("debug", Route::Debug);
#endif
}

/*****************************************************************************/
bool ArgumentPatterns::resolveFromArguments(const StringList& inArguments)
{
	m_argumentMap.clear();

	bool parseArgs = false;

	if (inArguments.size() > 1) // expects "program [subcommand]"
	{
		bool isOption = false;
		for (auto& arg : inArguments)
		{
			std::ptrdiff_t i = &arg - &inArguments.front();
			if (i == 0)
				continue;

			if (isOption)
			{
				isOption = false;
				if (!String::startsWith('-', arg))
					continue;
			}

			ushort res = parseOption(arg);
			if (res == 2)
			{
				isOption = true;
				continue;
			}
			else if (res == 1)
			{
				continue;
			}

			Route route = getRouteFromString(arg);

			if (m_subCommands.find(route) != m_subCommands.end())
			{
				m_route = route;
				m_routeString = arg;
				m_ignoreIndex = i;
				makeParser();

				auto& command = m_subCommands.at(m_route);
				command(*this);

				parseArgs = true;
				break; // we have to pass inArguments to doParse, so break out of this loop
			}
		}
	}

	if (parseArgs)
	{
		return doParse(inArguments);
	}
	else
	{
		m_route = Route::Unknown;
		m_routeString.clear();
		makeParser();

		populateMainArguments();

		return doParse(inArguments);
	}
}

/*****************************************************************************/
ushort ArgumentPatterns::parseOption(const std::string& inString)
{
	chalet_assert(!m_optionPairsCache.empty(), "!m_optionPairsCache.empty()");

	ushort res = 0;

	if (String::equals(m_optionPairsCache, inString))
	{
		// anything that takes 2 args
		res = 2;
	}
	else if (String::startsWith('-', inString))
	{
		// one arg (ie. --save-schema)
		res = 1;
	}

	return res;
}

/*****************************************************************************/
Route ArgumentPatterns::getRouteFromString(const std::string& inValue)
{
	if (m_routeMap.find(inValue) != m_routeMap.end())
		return m_routeMap.at(inValue);

	return Route::Unknown;
}

/*****************************************************************************/
const ArgumentPatterns::ArgumentMap& ArgumentPatterns::arguments() const noexcept
{
	return m_argumentMap;
}

Route ArgumentPatterns::route() const noexcept
{
	return m_route;
}

/*****************************************************************************/
StringList ArgumentPatterns::getRouteList()
{
	StringList ret;
	for (auto& [cmd, _] : m_routeMap)
	{
		ret.emplace_back(cmd);
	}

	return ret;
}

/*****************************************************************************/
void ArgumentPatterns::makeParser()
{
	const std::string program = "chalet";

	m_parser = argparse::ArgumentParser(program, "", argparse::default_arguments::none);

	addHelpArg();
	addVersionArg();

	if (m_route != Route::Unknown && !m_routeString.empty())
	{
		m_parser.add_argument(m_routeString)
			.default_value(true)
			.required();

		m_argumentMap.emplace(m_routeString, MappedArgument{ ArgumentIdentifier::RouteString, true });
	}
}

/*****************************************************************************/
bool ArgumentPatterns::doParse(const StringList& inArguments)
{
	CHALET_TRY
	{
		bool containsHelp = List::contains(inArguments, std::string("-h")) || List::contains(inArguments, std::string("--help"));
		bool containsVersion = List::contains(inArguments, std::string("-v")) || List::contains(inArguments, std::string("--version"));
		if (inArguments.size() <= 1 || containsHelp)
		{
			return showHelp();
		}
		else if (containsVersion)
		{
			return showVersion();
		}

		CHALET_TRY
		{
			m_parser.parse_args(inArguments);
		}
		CHALET_CATCH(const std::runtime_error& err)
		{
#if defined(CHALET_EXCEPTIONS)
			CHALET_THROW(err);
#endif
		}

		if (m_routeString.empty())
		{
			CHALET_EXCEPT_ERROR("Invalid subcommand requested. See 'chalet --help'.");
			return false;
		}
	}
	CHALET_CATCH(const std::runtime_error& err)
	{
		std::string error(err.what());
		if (String::startsWith("Unknown argument", error))
			error += '.';

		CHALET_EXCEPT_ERROR("{} See 'chalet --help' or 'chalet <subcommand> --help'.", error);
		return false;
	}
	CHALET_CATCH(const std::exception& err)
	{
		std::string error(err.what());
		if (String::startsWith("Unknown argument", error))
			error += '.';

		CHALET_EXCEPT_ERROR("{} See 'chalet --help' or 'chalet <subcommand> --help'.", err.what());
		return false;
	}

	return populateArgumentMap(inArguments);
}

/*****************************************************************************/
bool ArgumentPatterns::showHelp()
{
	std::string help = getHelp();
	std::cout.write(help.data(), help.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();

	m_route = Route::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentPatterns::showVersion()
{
	std::string version = "Chalet version 0.3.3";
	std::cout.write(version.data(), version.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();

	m_route = Route::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentPatterns::populateArgumentMap(const StringList& inArguments)
{
	std::string lastValue;
	auto gatherRemaining = [&inArguments, this](const std::string& inLastValue) -> StringList {
		StringList remaining;
		bool getNext = false;
		for (const auto& arg : inArguments)
		{
			std::ptrdiff_t i = &arg - &inArguments.front();
			if (i == m_ignoreIndex)
				continue;

			if (getNext)
			{
				remaining.push_back(arg);
				continue;
			}

			if (arg == inLastValue)
				getNext = true;
		}

		return remaining;
	};

	if (m_route != Route::Run && m_route != Route::BuildRun)
	{
		for (auto& arg : inArguments)
		{
			bool containsArgument = m_argumentMap.find(arg) != m_argumentMap.end();
			if (arg.front() == '-' && !containsArgument)
			{
				CHALET_EXCEPT_ERROR("An invalid argument was found: '{}'. See 'chalet --help' or 'chalet <subcommand> --help'.", arg);
				return false;
			}
		}
	}

	for (auto& [key, arg] : m_argumentMap)
	{
		if (key == m_routeString)
			continue;

		if (String::startsWith('-', key) && !List::contains(inArguments, key) && arg.value.kind() != Variant::Kind::Boolean)
			continue;

		CHALET_TRY
		{
			switch (arg.value.kind())
			{
				case Variant::Kind::Boolean: {
					arg.value = m_parser.get<bool>(key);
					break;
				}

				case Variant::Kind::OptionalBoolean: {
					std::string tmpValue = m_parser.get<std::string>(key);
					if (!tmpValue.empty())
					{
						int rawValue = atoi(tmpValue.c_str());
						arg.value = std::optional<bool>(rawValue == 1);
					}
					break;
				}

				case Variant::Kind::Integer: {
					std::string tmpValue = m_parser.get<std::string>(key);
					arg.value = atoi(tmpValue.c_str());
					break;
				}

				case Variant::Kind::OptionalInteger: {
					std::string tmpValue = m_parser.get<std::string>(key);
					if (!tmpValue.empty())
					{
						arg.value = std::optional<int>(atoi(tmpValue.c_str()));
					}
					break;
				}

				case Variant::Kind::String: {
					arg.value = m_parser.get<std::string>(key);
					lastValue = arg.value.asString();
					break;
				}

				case Variant::Kind::StringList: {
					CHALET_TRY
					{
						arg.value = m_parser.get<StringList>(key);
					}
					CHALET_CATCH(const std::exception& err)
					{
						CHALET_EXCEPT_ERROR(err.what());
					}
					return true;
				}

				case Variant::Kind::Remainder: {
					arg.value = gatherRemaining(lastValue);
					return true;
				}

				case Variant::Kind::Empty:
				default:
					break;
			}
		}
		CHALET_CATCH(const std::exception& err)
		{
			CHALET_EXCEPT_ERROR("Error parsing argument: {}. See 'chalet --help' or 'chalet <subcommand> --help'.", err.what());
			return false;
		}
	}

	return true;
}

/*****************************************************************************/
std::string ArgumentPatterns::getHelp()
{
	std::string title = "Chalet - A cross-platform JSON-based project & build tool";

	std::string help = m_parser.help().str();
	String::replaceAll(help, " [options] <subcommand>", " <subcommand> [options]");
	String::replaceAll(help, "Usage: ", "Usage:\n   ");
	String::replaceAll(help, "Positional arguments:", "Commands:");
	String::replaceAll(help, "Optional arguments:", "Options:");
	String::replaceAll(help, "\n<subcommand>", "");
	// String::replaceAll(help, "-h --help", "   -h --help");
	// String::replaceAll(help, "-v --version", "   -v --version");
	String::replaceAll(help, "[default: \"\"]", "");
	String::replaceAll(help, "[default: 0]", "");
	String::replaceAll(help, "[default: true]", "");
	String::replaceAll(help, "[default: false]", "");
	String::replaceAll(help, "[default: <not representable>]", "");

	help = fmt::format("{}\n\n{}", title, help);
	if (m_route != Route::Unknown)
	{
		RegexPatterns::matchAndReplace(help, "(\\[options\\]) (\\w+)", "$& [opts]");
		String::replaceAll(help, " [options]", "");
		String::replaceAll(help, "[opts]", "[options]");
	}

	if (m_route == Route::SettingsGetKeys)
	{
		String::replaceAll(help, fmt::format("{} {}", Arg::SettingsKeyQuery, Arg::RemainingArguments), Arg::SettingsKeyQuery);
		String::replaceAll(help, fmt::format("\n{}     	RMV", Arg::RemainingArguments), "");
	}

	if (String::contains("--toolchain", help))
	{
		auto getPresetMessage = [](const std::string& preset) -> std::string {
			if (String::equals("apple-llvm", preset))
				return fmt::format("Apple{} LLVM (Requires Xcode or \"Command Line Tools for Xcode\")", Unicode::registered());
			else if (String::equals("llvm", preset))
				return std::string("The LLVM Project");
			else if (String::equals("gcc", preset))
				return std::string("GNU Compiler Collection");
			else if (String::equals("intel-classic", preset))
				return fmt::format("Intel{} C++ Compiler Classic (for x86_64 processors)", Unicode::registered());

			return std::string();
		};

		const auto& defaultToolchain = m_inputs.defaultToolchainPreset();

		help += "\nToolchain Presets:\n";
		const auto toolchains = m_inputs.getToolchainPresets();
		for (auto& toolchain : toolchains)
		{
			const bool isDefault = String::equals(defaultToolchain, toolchain);
			std::string line = toolchain;
			while (line.size() < 28)
				line += ' ';
			line += '\t';
			line += getPresetMessage(toolchain);
			if (isDefault)
				line += " [default]";

			help += fmt::format("{}\n", line);
		}
	}

	return help;
}

/*****************************************************************************/
/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addStringArgument(const ArgumentIdentifier inId, const char* inArgument)
{
	auto& arg = m_parser.add_argument(inArgument);

	m_argumentMap.emplace(inArgument, MappedArgument{ inId, Variant::Kind::String });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addStringArgument(const ArgumentIdentifier inId, const char* inArgument, std::string inDefaultValue)
{
	auto& arg = m_parser.add_argument(inArgument)
					.default_value(std::move(inDefaultValue));

	m_argumentMap.emplace(inArgument, MappedArgument{ inId, Variant::Kind::String });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.nargs(1)
					.default_value(std::move(inDefaultValue));

	m_argumentMap.emplace(inShort, MappedArgument{ inId, Variant::Kind::String });
	m_argumentMap.emplace(inLong, MappedArgument{ inId, Variant::Kind::String });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.nargs(2);

	m_argumentMap.emplace(inShort, MappedArgument{ inId, Variant::Kind::OptionalInteger });
	m_argumentMap.emplace(inLong, MappedArgument{ inId, Variant::Kind::OptionalInteger });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue)
{
	auto& arg = m_parser.add_argument(inArgument)
					.nargs(1)
					.default_value(inDefaultValue)
					.implicit_value(true);

	m_argumentMap.emplace(inArgument, MappedArgument{ inId, Variant::Kind::Boolean });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument)
{
	auto& arg = m_parser.add_argument(inArgument)
					.nargs(1)
					.default_value(std::string());

	m_argumentMap.emplace(inArgument, MappedArgument{ inId, Variant::Kind::OptionalBoolean });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.nargs(1)
					.default_value(inDefaultValue)
					.implicit_value(true);

	m_argumentMap.emplace(inShort, MappedArgument{ inId, Variant::Kind::Boolean });
	m_argumentMap.emplace(inLong, MappedArgument{ inId, Variant::Kind::Boolean });

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addRemainingArguments(const ArgumentIdentifier inId, const char* inArgument)
{
	auto& arg = m_parser.add_argument(inArgument)
					.remaining();

	m_argumentMap.emplace(inArgument, MappedArgument{ inId, Variant::Kind::Remainder });

	return arg;
}

/*****************************************************************************/
void ArgumentPatterns::populateMainArguments()
{
	auto help = fmt::format(R"(
init [{path}]
configure
buildrun {runTarget} {runArgs}
run {runTarget} {runArgs}
build
rebuild
clean
bundle
get {key}
getkeys {keyQuery}
set {key} {value}
unset {key}
query {queryType} {queryArgs})",
		fmt::arg("runTarget", Arg::RunTarget),
		fmt::arg("runArgs", Arg::RemainingArguments),
		fmt::arg("key", Arg::SettingsKey),
		fmt::arg("keyQuery", Arg::SettingsKeyQuery),
		fmt::arg("value", Arg::SettingsValue),
		fmt::arg("path", Arg::InitPath),
		fmt::arg("queryType", Arg::QueryType),
		fmt::arg("queryArgs", Arg::RemainingArguments));

	m_parser.add_argument("<subcommand>")
		.help(std::move(help));
}

/*****************************************************************************/
void ArgumentPatterns::addHelpArg()
{
	m_parser.add_argument("-h", "--help")
		.default_value(false)
		.help("Shows help message for the subcommand (if applicable) and exits.")
		.implicit_value(true)
		.nargs(0);
}

/*****************************************************************************/
void ArgumentPatterns::addVersionArg()
{
	m_parser.add_argument("-v", "--version")
		.default_value(false)
		.help("Prints version information and exits.")
		.implicit_value(true)
		.nargs(0);
}

/*****************************************************************************/
void ArgumentPatterns::addInputFileArg()
{
	const auto& defaultValue = m_inputs.defaultInputFile();
	addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file")
		.help(fmt::format("An input build file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsFileArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	addTwoStringArguments(ArgumentIdentifier::SettingsFile, "-s", "--settings-file")
		.help(fmt::format("The path to a settings file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::File, "-f", "--file")
		.help("The path to a JSON file to examine, if not the local/global settings.");
}

/*****************************************************************************/
void ArgumentPatterns::addRootDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::RootDirectory, "-r", "--root-dir")
		.help("The root directory to run the build from. [default: \".\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addOutputDirArg()
{
	const auto& defaultValue = m_inputs.defaultOutputDirectory();
	addTwoStringArguments(ArgumentIdentifier::OutputDirectory, "-o", "--output-dir")
		.help(fmt::format("The output directory of the build. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addExternalDirArg()
{
	const auto& defaultValue = m_inputs.defaultExternalDirectory();
	addTwoStringArguments(ArgumentIdentifier::ExternalDirectory, "-x", "--external-dir")
		.help(fmt::format("The directory to install external dependencies into. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addDistributionDirArg()
{
	const auto& defaultValue = m_inputs.defaultDistributionDirectory();
	addTwoStringArguments(ArgumentIdentifier::DistributionDirectory, "-d", "--distribution-dir")
		.help(fmt::format("The root directory for all distribution bundles. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addProjectGenArg()
{
// future
#if 0
	addTwoStringArguments("-p", "--project-gen")
		.help("Project file generator [vs2019,vscode,xcode]");
#endif
}

/*****************************************************************************/
void ArgumentPatterns::addToolchainArg()
{
	const auto& defaultValue = m_inputs.defaultToolchainPreset();
	addTwoStringArguments(ArgumentIdentifier::Toolchain, "-t", "--toolchain")
		.help(fmt::format("A toolchain or toolchain preset to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addMaxJobsArg()
{
	auto jobs = std::thread::hardware_concurrency();
	addTwoIntArguments(ArgumentIdentifier::MaxJobs, "-j", "--max-jobs")
		.help(fmt::format("The number of jobs to run during compilation. [default: {}]", jobs));
}

/*****************************************************************************/
void ArgumentPatterns::addEnvFileArg()
{
	const auto& defaultValue = m_inputs.defaultEnvFile();
	addTwoStringArguments(ArgumentIdentifier::EnvFile, "-e", "--env-file")
		.help(fmt::format("A file to load environment variables from. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentPatterns::addArchArg()
{
	addTwoStringArguments(ArgumentIdentifier::TargetArchitecture, "-a", "--arch", "auto")
		.help("The architecture to target for the build.")
		.action([](const std::string& value) -> std::string {
			// https://gcc.gnu.org/onlinedocs/gcc/x86-Options.html
			// Either parsed later (if MSVC) or passed directly to GNU compiler
			if (std::all_of(value.begin(), value.end(), [](char c) {
					return ::isalpha(c)
						|| ::isdigit(c)
						|| c == '-'
						|| c == ','
						|| c == '.'
#if defined(CHALET_WIN32)
						|| c == '='
#endif
						|| c == '_';
				}))
			{
				return value;
			}
			return std::string{ "auto" };
		});
}

/*****************************************************************************/
void ArgumentPatterns::addSaveSchemaArg()
{
	addBoolArgument(ArgumentIdentifier::SaveSchema, "--save-schema", false)
		.help("Save build & settings schemas to file.");
}

/*****************************************************************************/
void ArgumentPatterns::addQuietArgs()
{
	// TODO: other quiet flags
	// --quiet = build & initialization output (no descriptions or flare)
	// --quieter = just build output
	// --quietest = no output

	addBoolArgument(ArgumentIdentifier::Quieter, "--quieter", false)
		.help("Show only the build output.");
}

/*****************************************************************************/
void ArgumentPatterns::addBuildConfigurationArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildConfiguration, "-c", "--configuration")
		.help("The build configuration to use. [default: \"Release\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addRunTargetArg()
{
	addStringArgument(ArgumentIdentifier::RunTargetName, Arg::RunTarget, std::string())
		.help("An executable or script target to run.");
}

/*****************************************************************************/
void ArgumentPatterns::addRunArgumentsArg()
{
	addRemainingArguments(ArgumentIdentifier::RunTargetArguments, Arg::RemainingArguments)
		.help("The arguments to pass to the run target.");
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsTypeArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	addTwoBoolArguments(ArgumentIdentifier::LocalSettings, "-l", "--local", false)
		.help(fmt::format("Use the local settings. [{}]", defaultValue));

	const auto& globalSettings = m_inputs.globalSettingsFile();
	addTwoBoolArguments(ArgumentIdentifier::GlobalSettings, "-g", "--global", false)
		.help(fmt::format("Use the global settings. [~/{}]", globalSettings));
}

/*****************************************************************************/
void ArgumentPatterns::addDumpAssemblyArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::DumpAssembly, "--dump-assembly")
		.help("Create an .asm dump of each object file during the build.");
}

/*****************************************************************************/
void ArgumentPatterns::addGenerateCompileCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::GenerateCompileCommands, "--generate-compile-commands")
		.help("Generate a compile_commands.json file for Clang tooling use.");
}

/*****************************************************************************/
void ArgumentPatterns::addShowCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::ShowCommands, "--show-commands")
		.help("Show the commands run during the build.");
}

/*****************************************************************************/
void ArgumentPatterns::addBenchmarkArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::Benchmark, "--benchmark")
		.help("Show all build times - total build time, build targets, other steps.");
}

/*****************************************************************************/
void ArgumentPatterns::addLaunchProfilerArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::LaunchProfiler, "--launch-profiler")
		.help("If running profile targets, launch the preferred profiler afterwards.");
}

/*****************************************************************************/
void ArgumentPatterns::addKeepGoingArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::KeepGoing, "--keep-going")
		.help("If there's a build error, continue as much of the build as possible.");
}

/*****************************************************************************/
void ArgumentPatterns::addOptionalArguments()
{
	addInputFileArg();
	addSettingsFileArg();
	addRootDirArg();
	addExternalDirArg();
	addOutputDirArg();
	addDistributionDirArg();
	addBuildConfigurationArg();
	addToolchainArg();
	addArchArg();
	addEnvFileArg();
	addProjectGenArg();
	addMaxJobsArg();
	addShowCommandsArg();
	addDumpAssemblyArg();
	addBenchmarkArg();
	addLaunchProfilerArg();
	addKeepGoingArg();
	addGenerateCompileCommandsArg();
#if defined(CHALET_DEBUG)
	addSaveSchemaArg();
#endif
	addQuietArgs();
}

/*****************************************************************************/
void ArgumentPatterns::populateBuildRunArguments()
{
	addOptionalArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::populateRunArguments()
{
	addOptionalArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::populateBuildArguments()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::populateRebuildArguments()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::populateCleanArguments()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::populateBundleArguments()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::populateConfigureArguments()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::populateInitArguments()
{
	const auto templates = m_inputs.getProjectInitializationPresets();
	addTwoStringArguments(ArgumentIdentifier::InitTemplate, "-t", "--template")
		.help(fmt::format("The project template to use during initialization. (ex: {})", String::join(templates, ", ")));

	addStringArgument(ArgumentIdentifier::InitPath, Arg::InitPath, ".")
		.help("The path of the project to initialize.")
		.required();
}

/*****************************************************************************/
void ArgumentPatterns::populateSettingsGetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, Arg::SettingsKey, std::string())
		.help("The config key to get.");
}

/*****************************************************************************/
void ArgumentPatterns::populateSettingsGetKeysArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, Arg::SettingsKeyQuery, std::string())
		.help("The config key to query for.");

	addRemainingArguments(ArgumentIdentifier::SettingsKeysRemainingArgs, Arg::RemainingArguments)
		.help("RMV");
}

/*****************************************************************************/
void ArgumentPatterns::populateSettingsSetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, Arg::SettingsKey)
		.help("The config key to change.")
		.required();

	addStringArgument(ArgumentIdentifier::SettingsValue, Arg::SettingsValue, std::string())
		.help("The config value to change to.");
}

/*****************************************************************************/
void ArgumentPatterns::populateSettingsUnsetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, Arg::SettingsKey)
		.help("The config key to remove.")
		.required();
}

/*****************************************************************************/
void ArgumentPatterns::populateQueryArguments()
{
	auto listNames = m_inputs.getCliQueryOptions();
	addStringArgument(ArgumentIdentifier::QueryType, Arg::QueryType)
		.help(fmt::format("The data type to query. ({})", String::join(listNames, ", ")))
		.required();

	addRemainingArguments(ArgumentIdentifier::QueryDataRemainingArgs, Arg::RemainingArguments)
		.help("Data to provide to the query. (architecture: <toolchain-name>)");
}

/*****************************************************************************/
void ArgumentPatterns::populateColorTestArguments()
{
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentPatterns::populateDebugArguments()
{
	addOptionalArguments();
}
#endif
}
