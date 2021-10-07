/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentPatterns.hpp"

#include <thread>

// #include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
MappedArgument::MappedArgument(ArgumentIdentifier inId, Variant inValue) :
	id(inId),
	value(std::move(inValue))
{
}

/*****************************************************************************/
ArgumentPatterns::ArgumentPatterns() :
	m_subCommands({
		{ Route::BuildRun, &ArgumentPatterns::commandBuildRun },
		{ Route::Run, &ArgumentPatterns::commandRun },
		{ Route::Build, &ArgumentPatterns::commandBuild },
		{ Route::Rebuild, &ArgumentPatterns::commandRebuild },
		{ Route::Clean, &ArgumentPatterns::commandClean },
		{ Route::Bundle, &ArgumentPatterns::commandBundle },
		{ Route::Configure, &ArgumentPatterns::commandConfigure },
		{ Route::Init, &ArgumentPatterns::commandInit },
		{ Route::SettingsGet, &ArgumentPatterns::commandSettingsGet },
		{ Route::SettingsGetKeys, &ArgumentPatterns::commandSettingsGetKeys },
		{ Route::SettingsSet, &ArgumentPatterns::commandSettingsSet },
		{ Route::SettingsUnset, &ArgumentPatterns::commandSettingsUnset },
		{ Route::List, &ArgumentPatterns::commandList },
	}),
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
		{ "list", Route::List },
	}),
	kArgRunTarget("[<runTarget>]"),
	kArgRemainingArguments("[ARG...]"),
	kArgInitName("<name>"),
	kArgInitPath("<path>"),
	kArgSettingsKey("<key>"),
	kArgSettingsKeyQuery("<query>"),
	kArgSettingsValue("<value>")
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(Route::Debug, &ArgumentPatterns::commandDebug);
	m_routeMap.emplace("debug", Route::Debug);
#endif
}

/*****************************************************************************/
bool ArgumentPatterns::parse(const StringList& inArguments)
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
	ushort res = 0;

	if (String::equals(m_optionPairsCache, inString))
	{
		// anything that takes 2 args
		res = 2;
	}
	else if (String::startsWith('-', inString))
	{
		// one arg (--save-schema)
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
	const std::string versionString = "chalet version 0.0.1";

	m_parser = argparse::ArgumentParser(program, versionString);
	// m_parser.add_description("Here is a description");
	// m_parser.add_epilog("\nHere is an epilog?");

	if (m_route == Route::Unknown || m_routeString.empty())
		return;

	m_parser.add_argument(m_routeString)
		.default_value(true)
		.required();

	m_argumentMap.emplace(m_routeString, MappedArgument{ ArgumentIdentifier::RouteString, true });
}

/*****************************************************************************/
bool ArgumentPatterns::doParse(const StringList& inArguments)
{
	CHALET_TRY
	{
		if (inArguments.size() <= 1)
		{
			// Diagnostic::error("No arguments were given");
			return showHelp();
		}

		if (List::contains(inArguments, std::string("-h")) || List::contains(inArguments, std::string("--help")))
		{
			// Diagnostic::error("help");
			return showHelp();
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
			// Diagnostic::error("No sub-command given");
			return showHelp();
		}
	}
	CHALET_CATCH(const std::runtime_error& err)
	{
		// LOG(err.what());
		showHelp();
		CHALET_EXCEPT_ERROR("{}", err.what());
		return false;
	}
	CHALET_CATCH(const std::exception& err)
	{
		CHALET_EXCEPT_ERROR("There was an unhandled exception during argument parsing: {}", err.what());
		return false;
	}

	return populateArgumentMap(inArguments);
}
/*****************************************************************************/
bool ArgumentPatterns::showHelp()
{
	std::string help = getHelp();
	// std::cout << err.what() << std::endl;
	std::cout << help << std::endl;
	m_route = Route::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentPatterns::populateArgumentMap(const StringList& inArguments)
{
	std::string lastValue;
	auto gatherRemaining = [&inArguments](const std::string& inLastValue) -> StringList {
		StringList remaining;
		bool getNext = false;
		for (const auto& arg : inArguments)
		{
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

	bool isRun = m_route == Route::Run || m_route == Route::BuildRun;
	for (auto& arg : inArguments)
	{
		bool containsArgument = m_argumentMap.find(arg) != m_argumentMap.end();
		if (arg.front() == '-' && !containsArgument && !isRun)
		{
			Diagnostic::error("An invalid argument was found: '{}'", arg);
			return false;
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
				case Variant::Kind::Boolean:
					arg.value = m_parser.get<bool>(key);
					break;

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

				case Variant::Kind::String:
					arg.value = m_parser.get<std::string>(key);
					lastValue = arg.value.asString();
					break;

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

				case Variant::Kind::Remainder:
					arg.value = gatherRemaining(lastValue);
					return true;

				case Variant::Kind::Empty:
				default:
					break;
			}
		}
		CHALET_CATCH(const std::exception& err)
		{
			Diagnostic::error("Error parsing argument: {}", err.what());
			Diagnostic::error("An invalid set of arguments were found.\n   Aborting...");
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

	std::string ret = fmt::format("{}\n\n{}", title, help);
	if (m_route == Route::Unknown)
	{
		ret += "\nAdditional help is available in each subcommand";
	}
	else
	{
		RegexPatterns::matchAndReplace(ret, "(\\[options\\]) (\\w+)", "$& [opts]");
		String::replaceAll(ret, " [options]", "");
		String::replaceAll(ret, "[opts]", "[options]");
	}

	if (m_route == Route::SettingsGetKeys)
	{
		String::replaceAll(ret, fmt::format("{} {}", kArgSettingsKeyQuery, kArgRemainingArguments), kArgSettingsKeyQuery);
		String::replaceAll(ret, fmt::format("\n{}     	RMV", kArgRemainingArguments), "");
	}

	return ret;
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

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.nargs(2);

	m_argumentMap.emplace(inShort, MappedArgument{ inId, Variant::Kind::OptionalInteger });
	m_argumentMap.emplace(inLong, MappedArgument{ inId, Variant::Kind::OptionalInteger });

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

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

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

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
   list)",
		fmt::arg("runTarget", kArgRunTarget),
		fmt::arg("runArgs", kArgRemainingArguments),
		fmt::arg("key", kArgSettingsKey),
		fmt::arg("keyQuery", kArgSettingsKeyQuery),
		fmt::arg("value", kArgSettingsValue),
		fmt::arg("path", kArgInitPath));

	m_parser.add_argument("<subcommand>")
		.help(std::move(help));
}

/*****************************************************************************/
void ArgumentPatterns::addInputFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file")
		.help("An input build file to use [default: \"chalet.json\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::SettingsFile, "-s", "--settings-file")
		.help("The path to a settings file to use [default: \".chaletrc\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::File, "-f", "--file")
		.help("The path to a JSON file to examine, if not the local/global settings");
}

/*****************************************************************************/
void ArgumentPatterns::addRootDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::RootDirectory, "-r", "--root-dir")
		.help("The root directory to run the build from");
}

/*****************************************************************************/
void ArgumentPatterns::addOutputDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::OutputDirectory, "-o", "--output-dir")
		.help("The output directory of the build [default: \"build\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addExternalDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::ExternalDirectory, "-x", "--external-dir")
		.help("The directory to install external dependencies into [default: \"chalet_external\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addBundleDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::DistributionDirectory, "-d", "--distribution-dir")
		.help("The root directory for all distribution bundles [default: \"dist\"]");
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
	addTwoStringArguments(ArgumentIdentifier::Toolchain, "-t", "--toolchain")
#if defined(CHALET_WIN32)
		.help("Toolchain preference (vs-stable, vs-preview, vs-2022 .. vs-2010, llvm, gcc, ...) [default: \"vs-stable\"]");
#elif defined(CHALET_MACOS)
		.help("Toolchain preference (apple-llvm, llvm, gcc, ...) [default: \"apple-llvm\"]");
#else
		.help("Toolchain preference (llvm, gcc, ...) [default: \"gcc\"]");
#endif
}

/*****************************************************************************/
void ArgumentPatterns::addMaxJobsArg()
{
	auto jobs = std::thread::hardware_concurrency();
	addTwoIntArguments(ArgumentIdentifier::MaxJobs, "-j", "--max-jobs")
		.help(fmt::format("The number of jobs to run during compilation [default: {}]", jobs));
}

/*****************************************************************************/
void ArgumentPatterns::addEnvFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::EnvFile, "-e", "--env-file")
		.help("A file to load environment variables from [default: \".env\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addArchArg()
{
	addTwoStringArguments(ArgumentIdentifier::TargetArchitecture, "-a", "--arch", "auto")
		.help("Target architecture")
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
		.help("Save build & settings schemas to file");
}

/*****************************************************************************/
void ArgumentPatterns::addQuietArgs()
{
	// TODO: other quiet flags
	// --quiet = build & initialization output (no descriptions or flare)
	// --quieter = just build output
	// --quietest = no output

	addBoolArgument(ArgumentIdentifier::Quieter, "--quieter", false)
		.help("Show only build output");
}

/*****************************************************************************/
void ArgumentPatterns::addBuildConfigurationArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildConfiguration, "-c", "--configuration")
		.help("The build configuration to use [default: \"Release\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addRunTargetArg()
{
	addStringArgument(ArgumentIdentifier::RunTargetName, kArgRunTarget.c_str(), std::string())
		.help("An executable or script target to run");
}

/*****************************************************************************/
void ArgumentPatterns::addRunArgumentsArg()
{
	addRemainingArguments(ArgumentIdentifier::RunTargetArguments, kArgRemainingArguments.c_str())
		.help("The arguments to pass to the run target");
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsTypeArg()
{
	addTwoBoolArguments(ArgumentIdentifier::LocalSettings, "-l", "--local", false)
		.help("Use the local settings [.chaletrc]");

	addTwoBoolArguments(ArgumentIdentifier::GlobalSettings, "-g", "--global", false)
		.help("Use the global settings [~/.chaletrc]");
}

/*****************************************************************************/
void ArgumentPatterns::addDumpAssemblyArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::DumpAssembly, "--dump-assembly")
		.help("Create an .asm dump of each object file during the build");
}

/*****************************************************************************/
void ArgumentPatterns::addGenerateCompileCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::GenerateCompileCommands, "--generate-compile-commands")
		.help("Generate a compile_commands.json file for Clang tooling use");
}

/*****************************************************************************/
void ArgumentPatterns::addShowCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::ShowCommands, "--show-commands")
		.help("Show the commands run during the build");
}

/*****************************************************************************/
void ArgumentPatterns::addBenchmarkArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::Benchmark, "--benchmark")
		.help("Show all build times (total build time, build targets, other steps)");
}

/*****************************************************************************/
void ArgumentPatterns::addOptionalArguments()
{
	addInputFileArg();
	addSettingsFileArg();
	addRootDirArg();
	addExternalDirArg();
	addOutputDirArg();
	addBuildConfigurationArg();
	addToolchainArg();
	addArchArg();
	addEnvFileArg();
	addProjectGenArg();
	addMaxJobsArg();
	addShowCommandsArg();
	addDumpAssemblyArg();
	addBenchmarkArg();
	addGenerateCompileCommandsArg();
	addSaveSchemaArg();
	addQuietArgs();
}

/*****************************************************************************/
void ArgumentPatterns::commandBuildRun()
{
	addOptionalArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRun()
{
	addOptionalArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandBuild()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::commandRebuild()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::commandClean()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::commandBundle()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::commandConfigure()
{
	addOptionalArguments();
}

/*****************************************************************************/
void ArgumentPatterns::commandInit()
{
	//
	addStringArgument(ArgumentIdentifier::InitPath, kArgInitPath.c_str(), ".")
		.help("The path of the project to initialize")
		.required();
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsGet()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, kArgSettingsKey.c_str(), std::string())
		.help("The config key to get");
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsGetKeys()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, kArgSettingsKeyQuery.c_str(), std::string())
		.help("The config key to query for");

	addRemainingArguments(ArgumentIdentifier::SettingsKeysRemainingArgs, kArgRemainingArguments.c_str())
		.help("RMV");
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsSet()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, kArgSettingsKey.c_str())
		.help("The config key to change")
		.required();

	addStringArgument(ArgumentIdentifier::SettingsValue, kArgSettingsValue.c_str(), std::string())
		.help("The config value to change to");
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsUnset()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(ArgumentIdentifier::SettingsKey, kArgSettingsKey.c_str())
		.help("The config key to remove")
		.required();
}

/*****************************************************************************/
void ArgumentPatterns::commandList()
{
	addStringArgument(ArgumentIdentifier::ListType, "--type", std::string())
		.help("The data type to list (commands, configurations, toolchain-presets, user-toolchains, all-toolchains, architectures, list-names)")
		.nargs(1)
		.required();
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentPatterns::commandDebug()
{
	addOptionalArguments();
}
#endif

}
