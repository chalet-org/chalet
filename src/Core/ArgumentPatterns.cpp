/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/ArgumentPatterns.hpp"

#include "Builder/BuildManager.hpp"
#include "Libraries/Format.hpp"
// #include "State/CommandLineInputs.hpp"
#include "Router/Route.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
const std::string ArgumentPatterns::kArgConfiguration = "<configuration>";
const std::string ArgumentPatterns::kArgRunProject = "[<runProject>]";
const std::string ArgumentPatterns::kArgRunArguments = "[ARG...]";
const std::string ArgumentPatterns::kArgInitName = "<name>";
const std::string ArgumentPatterns::kArgInitPath = "<path>";

/*****************************************************************************/
std::string ArgumentPatterns::getHelpCommand()
{
	return fmt::format(R"(
   buildrun {config} {runProj} {runArgs}
   run {config} {runProj} {runArgs}
   build {config}
   rebuild {config}
   clean [{config}]
   bundle
   install
   configure
   init {name} {path})",
		fmt::arg("config", ArgumentPatterns::kArgConfiguration),
		fmt::arg("runProj", ArgumentPatterns::kArgRunProject),
		fmt::arg("runArgs", ArgumentPatterns::kArgRunArguments),
		fmt::arg("name", ArgumentPatterns::kArgInitName),
		fmt::arg("path", ArgumentPatterns::kArgInitPath));
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
		{ Route::Install, &ArgumentPatterns::commandInstall },
		{ Route::Configure, &ArgumentPatterns::commandConfigure },
		{ Route::Init, &ArgumentPatterns::commandInit },
	})
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(Route::Debug, &ArgumentPatterns::commandDebug);
#endif
}

/*****************************************************************************/
bool ArgumentPatterns::parse(const StringList& inArguments)
{
	m_argumentMap.clear();

	if (inArguments.size() > 1) // expects "program [subcommand]"
	{
		for (auto& arg : inArguments)
		{
			std::size_t i = &arg - &inArguments.front();
			if (i == 0)
				continue;

			if (String::startsWith("-", arg))
				continue;

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
Route ArgumentPatterns::getRouteFromString(const std::string& inValue)
{
	if (String::equals(inValue, "buildrun"))
	{
		return Route::BuildRun;
	}
	else if (String::equals(inValue, "run"))
	{
		return Route::Run;
	}
	else if (String::equals(inValue, "build"))
	{
		return Route::Build;
	}
	else if (String::equals(inValue, "rebuild"))
	{
		return Route::Rebuild;
	}
	else if (String::equals(inValue, "clean"))
	{
		return Route::Clean;
	}
	else if (String::equals(inValue, "bundle"))
	{
		return Route::Bundle;
	}
	else if (String::equals(inValue, "install"))
	{
		return Route::Install;
	}
	else if (String::equals(inValue, "configure"))
	{
		return Route::Configure;
	}
	else if (String::equals(inValue, "init"))
	{
		return Route::Init;
	}
#if defined(CHALET_DEBUG)
	else if (String::equals(inValue, "debug"))
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
	try
	{
		if (inArguments.size() <= 1)
		{
			throw std::runtime_error("No arguments were given");
		}

		if (List::contains(inArguments, std::string("-h")) || List::contains(inArguments, std::string("--help")))
		{
			throw std::runtime_error("help");
		}

		try
		{
			m_parser.parse_args(inArguments);
		}
		catch (const std::runtime_error& err)
		{
			if (strcmp(err.what(), "Unknown argument") != 0)
				throw err;
		}

		if (m_routeString.empty())
		{
			throw std::runtime_error("No sub-command given");
		}
	}
	catch (const std::runtime_error& err)
	{
		std::string help = getHelp();
		// std::cout << err.what() << std::endl;
		std::cout << help << std::endl;
		return false;
	}

	populateArgumentMap(inArguments);

	return true;
}

/*****************************************************************************/
void ArgumentPatterns::populateArgumentMap(const StringList& inArguments)
{
	std::string lastValue;
	auto gatherRemaining = [inArguments](const std::string& inLastValue) -> StringList {
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

	for (auto& [key, value] : m_argumentMap)
	{
		if (key == m_routeString)
			continue;

		if (String::startsWith("-", key) && !List::contains(inArguments, key))
			continue;

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
				try
				{
					value = m_parser.get<StringList>(key);
				}
				catch (std::exception& e)
				{
					std::cout << e.what() << std::endl;
				}
				return;
			}

			case Variant::Kind::Remainder:
				value = gatherRemaining(lastValue);
				return;

			case Variant::Kind::Empty:
			default:
				break;
		}
	}
}

/*****************************************************************************/
std::string ArgumentPatterns::getHelp()
{
	std::string help = m_parser.help().str();
	String::replaceAll(help, "Usage: ", "Usage:\n   ");
	String::replaceAll(help, "Positional arguments:", "Commands:");
	String::replaceAll(help, "Optional arguments:", "Options:");
	String::replaceAll(help, fmt::format("\n{}", kCommand), "");
	// String::replaceAll(help, "-h --help", "   -h --help");
	// String::replaceAll(help, "-v --version", "   -v --version");
	String::replaceAll(help, "[default: \"\"]", "");
	String::replaceAll(help, "[default: true]", "");

	return fmt::format("JSON C++ Build System\n\n{help}", FMT_ARG(help));
}

/*****************************************************************************/

/*
	Sub-Commands:
		chalet buildrun <configuration> [<runProject>] [ARG...]
		chalet run <configuration> [<runProject>] [ARG...]
		chalet build <configuration>
		chalet rebuild <configuration>
		chalet clean [<configuration>]
		chalet bundle
		chalet install

	chalet -h,--help
	chalet -v,--version
*/

/*****************************************************************************/
void ArgumentPatterns::populateMainArguments()
{
	m_parser.add_argument(kCommand)
		.help(getHelpCommand());
}

/*****************************************************************************/
void ArgumentPatterns::addInputFileArg()
{
	m_parser.add_argument("-i", "--input")
		.help("input file")
		.nargs(1)
		.default_value(std::string("build.json"));

	m_argumentMap.push_back({ "-i", Variant::Kind::String });
	m_argumentMap.push_back({ "--input", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addProjectGeneratorArg()
{
	m_parser.add_argument("-g", "--generator")
		.help("project file generator")
		.nargs(1)
		.default_value(std::string());

	m_argumentMap.push_back({ "-g", Variant::Kind::String });
	m_argumentMap.push_back({ "--generator", Variant::Kind::String });
}

/*****************************************************************************/
void ArgumentPatterns::addSaveSchemaArg()
{
	// This option should only be in the debug executable
#ifdef CHALET_DEBUG
	m_parser.add_argument("--save-schema")
		.help("save schema")
		.nargs(1)
		.default_value(false)
		.implicit_value(true);

	m_argumentMap.push_back({ "--save-schema", Variant::Kind::Boolean });
#endif
}

/*****************************************************************************/
void ArgumentPatterns::addBuildConfigurationArg(const bool inOptional)
{
	if (inOptional)
	{
		m_parser.add_argument(kArgConfiguration)
			.help(kHelpBuildConfiguration)
			.default_value(std::string(""));
	}
	else
	{
		m_parser.add_argument(kArgConfiguration)
			.help(kHelpBuildConfiguration)
			.required();
	}

	m_argumentMap.push_back({ kArgConfiguration, Variant::Kind::String });
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
void ArgumentPatterns::commandBuildRun()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();

	addBuildConfigurationArg();
	addRunProjectArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRun()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();

	addBuildConfigurationArg();
	addRunProjectArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandBuild()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();

	addBuildConfigurationArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandRebuild()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();

	addBuildConfigurationArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandClean()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();

	bool optional = true;
	addBuildConfigurationArg(optional);
}

/*****************************************************************************/
void ArgumentPatterns::commandBundle()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandInstall()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandConfigure()
{
	addInputFileArg();
	addProjectGeneratorArg();
	addSaveSchemaArg();
}

/*****************************************************************************/
void ArgumentPatterns::commandInit()
{
	addInputFileArg();
	addProjectGeneratorArg();

	//
	m_parser.add_argument(kArgInitName)
		.help(kHelpInitName)
		.required();

	m_argumentMap.push_back({ kArgInitName, Variant::Kind::String });

	//
	m_parser.add_argument(kArgInitPath)
		.help(kHelpInitPath)
		.required();

	m_argumentMap.push_back({ kArgInitPath, Variant::Kind::String });
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentPatterns::commandDebug()
{
	addInputFileArg();
	addProjectGeneratorArg();
}
#endif

}
