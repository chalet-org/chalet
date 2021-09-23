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
		{ Route::SettingsSet, &ArgumentPatterns::commandSettingsSet },
		{ Route::SettingsUnset, &ArgumentPatterns::commandSettingsUnset },
	})
// m_optionPairsCache({
// 	// clang-format off
// 	"-i", "--input-file",
// 	"-s", "--settings-file",
// 	"-f", "--file",
// 	"-r", "--root-dir",
// 	"-o", "--output-dir",
// 	"-x", "--external-dir",
// 	"-d", "--distribution-dir",
// 	"-t", "--toolchain",
// 	"-a", "--arch",
// 	"-c", "--configuration",
// 	"-j", "--max-jobs",
// 	// "-p", "--project-gen",
// 	"-e", "--env-file",
// 	"-l", "--local",
// 	"-g", "--global",
// 	// clang-format on
// })
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(Route::Debug, &ArgumentPatterns::commandDebug);
#endif
	// --dump-assembly
	// --generate-compile-commands=0
	// --show-commands=0
	// --benchmark=1
	// --signing-identity
}

/*****************************************************************************/
const std::string& ArgumentPatterns::argRunProject() const noexcept
{
	return kArgRunProject;
}
const std::string& ArgumentPatterns::argRunArguments() const noexcept
{
	return kArgRunArguments;
}
const std::string& ArgumentPatterns::argInitPath() const noexcept
{
	return kArgInitPath;
}
const std::string& ArgumentPatterns::argConfigKey() const noexcept
{
	return kArgConfigKey;
}
const std::string& ArgumentPatterns::argConfigValue() const noexcept
{
	return kArgConfigValue;
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
	if (String::equals("buildrun", inValue))
	{
		return Route::BuildRun;
	}
	else if (String::equals("run", inValue))
	{
		return Route::Run;
	}
	else if (String::equals("build", inValue))
	{
		return Route::Build;
	}
	else if (String::equals("rebuild", inValue))
	{
		return Route::Rebuild;
	}
	else if (String::equals("clean", inValue))
	{
		return Route::Clean;
	}
	else if (String::equals("bundle", inValue))
	{
		return Route::Bundle;
	}
	else if (String::equals("configure", inValue))
	{
		return Route::Configure;
	}
	else if (String::equals("init", inValue))
	{
		return Route::Init;
	}
	else if (String::equals("get", inValue))
	{
		return Route::SettingsGet;
	}
	else if (String::equals("set", inValue))
	{
		return Route::SettingsSet;
	}
	else if (String::equals("unset", inValue))
	{
		return Route::SettingsUnset;
	}
#if defined(CHALET_DEBUG)
	else if (String::equals("debug", inValue))
	{
		return Route::Debug;
	}
#endif

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

	m_argumentMap.emplace(m_routeString, true);
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

	StringList keys;
	for (auto& [key, value] : m_argumentMap)
	{
		keys.push_back(key);
	}

	bool isRun = m_route == Route::Run || m_route == Route::BuildRun;
	for (auto& arg : inArguments)
	{
		if (arg.front() == '-' && !List::contains(keys, arg) && !isRun)
		{
			Diagnostic::error("An invalid argument was found: '{}'", arg);
			return false;
		}
	}

	for (auto& [key, value] : m_argumentMap)
	{
		if (key == m_routeString)
			continue;

		if (String::startsWith('-', key) && !List::contains(inArguments, key) && value.kind() != Variant::Kind::Boolean)
			continue;

		CHALET_TRY
		{
			switch (value.kind())
			{
				case Variant::Kind::Boolean:
					value = m_parser.get<bool>(key);
					break;

				case Variant::Kind::Integer: {
					std::string tmpValue = m_parser.get<std::string>(key);
					value = atoi(tmpValue.c_str());
					break;
				}

				case Variant::Kind::String:
					value = m_parser.get<std::string>(key);
					lastValue = value.asString();
					break;

				case Variant::Kind::StringList: {
					CHALET_TRY
					{
						value = m_parser.get<StringList>(key);
					}
					CHALET_CATCH(std::exception & err)
					{
						CHALET_EXCEPT_ERROR(err.what());
					}
					return true;
				}

				case Variant::Kind::Remainder:
					value = gatherRemaining(lastValue);
					return true;

				case Variant::Kind::Empty:
				default:
					break;
			}
		}
		CHALET_CATCH(const std::exception&)
		{
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

	std::string ret = fmt::format("{title}\n\n{help}", FMT_ARG(title), FMT_ARG(help));
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

	return ret;
}

/*****************************************************************************/
/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addStringArgument(const char* inArgument, const char* inHelp)
{
	auto& arg = m_parser.add_argument(inArgument)
					.help(inHelp);

	m_argumentMap.emplace(inArgument, Variant::Kind::String);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addStringArgument(const char* inArgument, const char* inHelp, std::string inDefaultValue)
{
	auto& arg = m_parser.add_argument(inArgument)
					.help(inHelp)
					.default_value(std::move(inDefaultValue));

	m_argumentMap.emplace(inArgument, Variant::Kind::String);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoStringArguments(const char* inShort, const char* inLong, const char* inHelp, std::string inDefaultValue)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.help(inHelp)
					.nargs(1)
					.default_value(std::move(inDefaultValue));

	m_argumentMap.emplace(inShort, Variant::Kind::String);
	m_argumentMap.emplace(inLong, Variant::Kind::String);

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoIntArguments(const char* inShort, const char* inLong, const char* inHelp, const int inDefaultValue)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.help(inHelp)
					.nargs(1)
					.default_value(inDefaultValue);

	m_argumentMap.emplace(inShort, Variant::Kind::Integer);
	m_argumentMap.emplace(inLong, Variant::Kind::Integer);

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addBoolArgument(const char* inArgument, const char* inHelp, const bool inDefaultValue)
{
	auto& arg = m_parser.add_argument(inArgument)
					.help(inHelp)
					.nargs(1)
					.default_value(inDefaultValue)
					.implicit_value(true);

	m_argumentMap.emplace(inArgument, Variant::Kind::Boolean);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addTwoBoolArguments(const char* inShort, const char* inLong, const char* inHelp, const bool inDefaultValue)
{
	auto& arg = m_parser.add_argument(inShort, inLong)
					.help(inHelp)
					.nargs(1)
					.default_value(inDefaultValue)
					.implicit_value(true);

	m_argumentMap.emplace(inShort, Variant::Kind::Boolean);
	m_argumentMap.emplace(inLong, Variant::Kind::Boolean);

	m_optionPairsCache.emplace_back(inShort);
	m_optionPairsCache.emplace_back(inLong);

	return arg;
}

/*****************************************************************************/
argparse::Argument& ArgumentPatterns::addRemainingArguments(const char* inArgument, const char* inHelp)
{
	auto& arg = m_parser.add_argument(inArgument)
					.help(inHelp)
					.remaining();

	m_argumentMap.emplace(inArgument, Variant::Kind::Remainder);

	return arg;
}

/*****************************************************************************/
void ArgumentPatterns::populateMainArguments()
{
	m_parser.add_argument("<subcommand>")
		.help(fmt::format(R"(
   init [{path}]
   configure
   buildrun {runProject} {runArgs}
   run {runProject} {runArgs}
   build
   rebuild
   clean
   bundle
   get {key}
   set {key} {value}
   unset {key})",
			fmt::arg("runProject", kArgRunProject),
			fmt::arg("runArgs", kArgRunArguments),
			fmt::arg("key", kArgConfigKey),
			fmt::arg("value", kArgConfigValue),
			fmt::arg("path", kArgInitPath)));
}

/*****************************************************************************/
void ArgumentPatterns::addInputFileArg()
{
	addTwoStringArguments("-i", "--input-file", "An input build file to use [default: \"chalet.json\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsFileArg()
{
	addTwoStringArguments("-s", "--settings-file", "The path to a settings file to use [default: \".chaletrc\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addFileArg()
{
	addTwoStringArguments("-f", "--file", "The path to a JSON file to examine, if not the local/global settings");
}

/*****************************************************************************/
void ArgumentPatterns::addRootDirArg()
{
	addTwoStringArguments("-r", "--root-dir", "The root directory to run the build from");
}

/*****************************************************************************/
void ArgumentPatterns::addOutputDirArg()
{
	addTwoStringArguments("-o", "--output-dir", "The output directory of the build [default: \"build\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addExternalDirArg()
{
	addTwoStringArguments("-x", "--external-dir", "The directory to install external dependencies into [default: \"chalet_external\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addBundleDirArg()
{
	addTwoStringArguments("-d", "--distribution-dir", "The root directory for all distribution bundles [default: \"dist\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addProjectGenArg()
{
	// future
#if 0
	addTwoStringArguments("-p", "--project-gen", "Project file generator [vs2019,vscode,xcode]");
#endif
}

/*****************************************************************************/
void ArgumentPatterns::addToolchainArg()
{
	addTwoStringArguments("-t", "--toolchain",
#if defined(CHALET_WIN32)
		"Toolchain preference (msvc, msvc-pre, llvm, gcc, ...) [default: \"msvc\"]"
#elif defined(CHALET_MACOS)
		"Toolchain preference (apple-llvm, llvm, gcc, ...) [default: \"apple-llvm\"]"
#else
		"Toolchain preference (llvm, gcc, ...) [default: \"gcc\"]"
#endif
	);
}

/*****************************************************************************/
void ArgumentPatterns::addMaxJobsArg()
{
	auto jobs = std::thread::hardware_concurrency();
	auto help = fmt::format("The number of jobs to run during compilation [default: {}]", jobs);
	addTwoIntArguments("-j", "--max-jobs", help.c_str(), 0);
}

/*****************************************************************************/
void ArgumentPatterns::addEnvFileArg()
{
	addTwoStringArguments("-e", "--env-file", "A file to load environment variables from [default: \".env\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addArchArg()
{
	addTwoStringArguments("-a", "--arch", "Target architecture", "auto")
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
	addBoolArgument("--save-schema", "Save build & cache schemas to file", false);
}

/*****************************************************************************/
void ArgumentPatterns::addQuietArgs()
{
	// TODO: other quiet flags
	// --quiet = build & initialization output (no descriptions or flare)
	// --quieter = just build output
	// --quietest = no output

	addBoolArgument("--quieter", "Show only build output", false);
}

/*****************************************************************************/
void ArgumentPatterns::addBuildConfigurationArg()
{
	addTwoStringArguments("-c", "--configuration", "The build configuration to use [default: \"Release\"]");
}

/*****************************************************************************/
void ArgumentPatterns::addRunProjectArg()
{
	addStringArgument(kArgRunProject.c_str(), "A project to run", std::string());
}

/*****************************************************************************/
void ArgumentPatterns::addRunArgumentsArg()
{
	addRemainingArguments(kArgRunArguments.c_str(), "The arguments to pass to the run project");
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsTypeArg()
{
	addTwoBoolArguments("-l", "--local", "Use the local settings [.chaletrc]", true);
	addTwoBoolArguments("-g", "--global", "Use the global settings [~/.chaletrc]", false);
}

/*****************************************************************************/
void ArgumentPatterns::addDumpAssemblyArg()
{
	// addBoolArgument("--dump-assembly", "Include an .asm dump of each object file during the build", false);
}

/*****************************************************************************/
void ArgumentPatterns::addGenerateCompileCommandsArg()
{
}

/*****************************************************************************/
void ArgumentPatterns::addShowCommandsArg()
{
}

/*****************************************************************************/
void ArgumentPatterns::addBenchmarkArg()
{
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

	addRunProjectArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRun()
{
	addOptionalArguments();

	addRunProjectArg();
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
	addStringArgument(kArgInitPath.c_str(), "The path of the project to initialize", ".")
		.required();
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsGet()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(kArgConfigKey.c_str(), "The config key to get", std::string());
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsSet()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(kArgConfigKey.c_str(), "The config key to change")
		.required();

	addStringArgument(kArgConfigValue.c_str(), "The config value to change to", std::string());
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsUnset()
{
	addFileArg();
	addSettingsTypeArg();

	addStringArgument(kArgConfigKey.c_str(), "The config key to remove")
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
