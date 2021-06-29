/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentPatterns.hpp"

// #include "Core/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
std::string ArgumentPatterns::getHelpCommand()
{
	return fmt::format(R"(
   buildrun {buildConfiguration} {runProject} {runArgs}
   run {buildConfiguration} {runProject} {runArgs}
   build {buildConfiguration}
   rebuild {buildConfiguration}
   clean [{buildConfiguration}]
   bundle
   configure
   get {key}
   set {key} {value}
   unset {key}
   init [{path}])",
		fmt::arg("buildConfiguration", kArgBuildConfiguration),
		fmt::arg("runProject", kArgRunProject),
		fmt::arg("runArgs", kArgRunArguments),
		fmt::arg("key", kArgConfigKey),
		fmt::arg("value", kArgConfigValue),
		fmt::arg("path", kArgInitPath));
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
		{ Route::SettingsSet, &ArgumentPatterns::commandSettingsSet },
		{ Route::SettingsUnset, &ArgumentPatterns::commandSettingsUnset },
	})
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(Route::Debug, &ArgumentPatterns::commandDebug);
#endif
}

/*****************************************************************************/
const std::string& ArgumentPatterns::argBuildConfiguration() const noexcept
{
	return kArgBuildConfiguration;
}
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

	if (inArguments.size() > 1) // expects "program [subcommand]"
	{
		bool isOption = false;
		for (auto& arg : inArguments)
		{
			std::size_t i = &arg - &inArguments.front();
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

				return doParse(inArguments);
			}
		}
	}

	m_route = Route::Unknown;
	m_routeString.clear();
	makeParser();

	populateMainArguments();

	return doParse(inArguments);
}

/*****************************************************************************/
ushort ArgumentPatterns::parseOption(const std::string& inString)
{
	ushort res = 0;

	// clang-format off
	if (String::equals({
		"-i", "--input-file",
		"-o", "--output-path",
		"-t", "--toolchain",
		"-g", "--generator",
		"-e", "--envfile",
		"-a", "--arch",
	}, inString))
	// clang-format on
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

	m_argumentMap.push_back({ m_routeString, true });
}

/*****************************************************************************/
bool ArgumentPatterns::doParse(const StringList& inArguments)
{
	CHALET_TRY
	{
		if (inArguments.size() <= 1)
		{
			// CHALET_THROW(std::runtime_error("No arguments were given"));
			return showHelp();
		}

		if (List::contains(inArguments, std::string("-h")) || List::contains(inArguments, std::string("--help")))
		{
			// CHALET_THROW(std::runtime_error("help"));
			return showHelp();
		}

		CHALET_TRY
		{
			m_parser.parse_args(inArguments);
		}
		CHALET_CATCH(const std::runtime_error& err)
		{
#if defined(CHALET_EXCEPTIONS)
			if (strcmp(err.what(), "Unknown argument") != 0)
				CHALET_THROW(err);
#endif
		}

		if (m_routeString.empty())
		{
			// CHALET_THROW(std::runtime_error("No sub-command given"));
			return showHelp();
		}
	}
	CHALET_CATCH(const std::runtime_error&)
	{
		return showHelp();
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
	return false;
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

	for (auto& arg : inArguments)
	{
		if (arg.front() == '-' && !List::contains(keys, arg))
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

				case Variant::Kind::Integer:
					value = m_parser.get<int>(key);
					break;

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
	std::string title = "Chalet: A build system for C & C++";
	std::string help = m_parser.help().str();
	String::replaceAll(help, "Usage: ", "Usage:\n   ");
	String::replaceAll(help, "Positional arguments:", "Commands:");
	String::replaceAll(help, "Optional arguments:", "Options:");
	String::replaceAll(help, fmt::format("\n{}", kCommand), "");
	// String::replaceAll(help, "-h --help", "   -h --help");
	// String::replaceAll(help, "-v --version", "   -v --version");
	String::replaceAll(help, "[default: \"\"]", "");
	String::replaceAll(help, "[default: true]", "");

	return fmt::format("{title}\n\n{help}", FMT_ARG(title), FMT_ARG(help));
}

/*****************************************************************************/
/*****************************************************************************/
void ArgumentPatterns::populateMainArguments()
{
	m_parser.add_argument(kCommand)
		.help(getHelpCommand());
}

/*****************************************************************************/
void ArgumentPatterns::addInputFileArg()
{
	m_parser.add_argument("-i", "--input-file")
		.help("Input file")
		.nargs(1)
		.default_value(std::string("build.json"));

	m_argumentMap.push_back({ "-i", Variant::Kind::String });
	m_argumentMap.push_back({ "--input-file", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addOutPathArg()
{
	m_parser.add_argument("-o", "--output-path")
		.help("Output build path")
		.nargs(1)
		.default_value(std::string("build"));

	m_argumentMap.push_back({ "-o", Variant::Kind::String });
	m_argumentMap.push_back({ "--output-path", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addProjectGeneratorArg()
{
	m_parser.add_argument("-g", "--generator")
		.help("Project file generator [vs2019,vscode,xcode]")
		.nargs(1)
		.default_value(std::string());

	m_argumentMap.push_back({ "-g", Variant::Kind::String });
	m_argumentMap.push_back({ "--generator", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addToolchainArg()
{
	m_parser.add_argument("-t", "--toolchain")
#if defined(CHALET_WIN32)
		.help("Toolchain preference [msvc,llvm,gcc,...]")
#else
		.help("Toolchain preference [msvc,llvm,gcc,...]")
#endif
		.nargs(1)
		.default_value(std::string());

	m_argumentMap.push_back({ "-t", Variant::Kind::String });
	m_argumentMap.push_back({ "--toolchain", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addEnvFileArg()
{
	m_parser.add_argument("-e", "--envfile")
		.help("File to load environment variables from")
		.nargs(1)
		.default_value(std::string());

	m_argumentMap.push_back({ "-e", Variant::Kind::String });
	m_argumentMap.push_back({ "--envfile", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addArchArg()
{
	m_parser.add_argument("-a", "--arch")
		.help("Target architecture")
		.nargs(1)
		.default_value(std::string("auto"))
		.action([](const std::string& value) {
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

	m_argumentMap.push_back({ "-a", Variant::Kind::String });
	m_argumentMap.push_back({ "--arch", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addSaveSchemaArg()
{
	m_parser.add_argument("--save-schema")
		.help("Save build & cache schemas to file")
		.nargs(1)
		.default_value(false)
		.implicit_value(true);

	m_argumentMap.push_back({ "--save-schema", Variant::Kind::Boolean });
}

/*****************************************************************************/
void ArgumentPatterns::addQuietArgs()
{
	// TODO: other quiet flags
	// --quiet = build & initialization output (no descriptions or flare)
	// --quieter = just build output
	// --quietest = no output

	m_parser.add_argument("--quieter")
		.help("Show only build output")
		.nargs(1)
		.default_value(false)
		.implicit_value(true);

	m_argumentMap.push_back({ "--quieter", Variant::Kind::Boolean });
}

/*****************************************************************************/
void ArgumentPatterns::addBuildConfigurationArg(const bool inOptional)
{
	if (inOptional)
	{
		m_parser.add_argument(kArgBuildConfiguration)
			.help(kHelpBuildConfiguration)
			.default_value(std::string(""));
	}
	else
	{
		m_parser.add_argument(kArgBuildConfiguration)
			.help(kHelpBuildConfiguration)
			.required();
	}

	m_argumentMap.push_back({ kArgBuildConfiguration, Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addRunProjectArg()
{
	m_parser.add_argument(kArgRunProject)
		.help(kHelpRunProject)
		.default_value(std::string(""));

	m_argumentMap.push_back({ kArgRunProject, Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addRunArgumentsArg()
{
	m_parser.add_argument(kArgRunArguments)
		.help(kHelpRunArguments)
		.remaining();

	m_argumentMap.push_back({ kArgRunArguments, Variant::Kind::Remainder });
}

/*****************************************************************************/
void ArgumentPatterns::addSettingsTypeArg()
{
	m_parser.add_argument("--local")
		.help("use the local cache")
		.nargs(1)
		.default_value(true)
		.implicit_value(true);

	m_argumentMap.push_back({ "--local", Variant::Kind::Boolean });

	m_parser.add_argument("--global")
		.help("use the global cache")
		.nargs(1)
		.default_value(false)
		.implicit_value(true);

	m_argumentMap.push_back({ "--global", Variant::Kind::Boolean });
}

/*****************************************************************************/
void ArgumentPatterns::commandBuildRun()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();

	addBuildConfigurationArg();
	addRunProjectArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRun()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();

	addBuildConfigurationArg();
	addRunProjectArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandBuild()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();

	addBuildConfigurationArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRebuild()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();

	addBuildConfigurationArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandClean()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();

	bool optional = true;
	addBuildConfigurationArg(optional);
}

/*****************************************************************************/
void ArgumentPatterns::commandBundle()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();
}

/*****************************************************************************/
void ArgumentPatterns::commandConfigure()
{
	addInputFileArg();
	addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
	addSaveSchemaArg();
	addQuietArgs();
}

/*****************************************************************************/
void ArgumentPatterns::commandInit()
{
	addInputFileArg();
	// addOutPathArg();
	addToolchainArg();
	addProjectGeneratorArg();
	addEnvFileArg();

	//
	m_parser.add_argument(kArgInitPath)
		.help(kHelpInitPath)
		.default_value(std::string("."))
		.required();

	m_argumentMap.push_back({ kArgInitPath, Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsGet()
{
	addSettingsTypeArg();

	m_parser.add_argument(kArgConfigKey)
		.help("The config key to get")
		.default_value(std::string(""));

	m_argumentMap.push_back({ kArgConfigKey, Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsSet()
{
	addSettingsTypeArg();

	m_parser.add_argument(kArgConfigKey)
		.help("The config key to change")
		.required();

	m_argumentMap.push_back({ kArgConfigKey, Variant::Kind::String });

	m_parser.add_argument(kArgConfigValue)
		.help("The config value to change to")
		.default_value(std::string(""));

	m_argumentMap.push_back({ kArgConfigValue, Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::commandSettingsUnset()
{
	addSettingsTypeArg();

	m_parser.add_argument(kArgConfigKey)
		.help("The config key to remove")
		.required();

	m_argumentMap.push_back({ kArgConfigKey, Variant::Kind::String });
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentPatterns::commandDebug()
{
	addInputFileArg();
	addOutPathArg();
	addProjectGeneratorArg();
	addEnvFileArg();
	addArchArg();
}
#endif

}
