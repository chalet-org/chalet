/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Arguments/ArgumentParser.hpp"

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
CH_STR(RunTarget) = "[<target>]";
CH_STR(RemainingArguments) = "[ARG...]";
// CH_STR(InitName) = "<name>";
CH_STR(InitPath) = "<path>";
CH_STR(ExportKind) = "<kind>";
CH_STR(SettingsKey) = "<key>";
CH_STR(SettingsKeyQuery) = "<query>";
CH_STR(SettingsValue) = "<value>";
CH_STR(QueryType) = "<type>";
// CH_STR(QueryData) = "<data>";
}

namespace Positional
{
CH_STR(ProgramArgument) = "@0";
CH_STR(Argument1) = "@1";
CH_STR(Argument2) = "@2";
CH_STR(RemainingArguments) = "...";
}

#undef CH_STR

/*****************************************************************************/
ArgumentParser::ArgumentParser(const CommandLineInputs& inInputs) :
	BaseArgumentParser(),
	m_inputs(inInputs),
	m_subCommands({
		{ Route::BuildRun, &ArgumentParser::populateBuildRunArguments },
		{ Route::Run, &ArgumentParser::populateRunArguments },
		{ Route::Build, &ArgumentParser::populateBuildArguments },
		{ Route::Rebuild, &ArgumentParser::populateBuildArguments },
		{ Route::Clean, &ArgumentParser::populateBuildArguments },
		{ Route::Bundle, &ArgumentParser::populateBuildArguments },
		{ Route::Configure, &ArgumentParser::populateBuildArguments },
		{ Route::Init, &ArgumentParser::populateInitArguments },
		{ Route::Export, &ArgumentParser::populateExportArguments },
		{ Route::SettingsGet, &ArgumentParser::populateSettingsGetArguments },
		{ Route::SettingsGetKeys, &ArgumentParser::populateSettingsGetKeysArguments },
		{ Route::SettingsSet, &ArgumentParser::populateSettingsSetArguments },
		{ Route::SettingsUnset, &ArgumentParser::populateSettingsUnsetArguments },
		{ Route::Query, &ArgumentParser::populateQueryArguments },
		{ Route::ColorTest, &ArgumentParser::populateColorTestArguments },
	}),
	m_routeDescriptions({
		{ Route::BuildRun, "Build the project and run a valid executable build target." },
		{ Route::Run, "Run a valid executable build target." },
		{ Route::Build, "Build the project and create a configuration if it doesn't exist." },
		{ Route::Rebuild, "Rebuild the project and create a configuration if it doesn't exist." },
		{ Route::Clean, "Unceremoniously clean the build folder." },
		{ Route::Bundle, "Bundle the project for distribution." },
		{ Route::Configure, "Create a project configuration and fetch external dependencies." },
		{ Route::Export, "Export the project to another project format." },
		{ Route::Init, "Initialize a project in either the current directory or a subdirectory." },
		{ Route::SettingsGet, "If the given property is valid, display its JSON node." },
		{ Route::SettingsGetKeys, "If the given property is an object, display the names of its properties." },
		{ Route::SettingsSet, "Set the given property to the given value." },
		{ Route::SettingsUnset, "Remove the key/value pair given a valid property key." },
		{ Route::Query, "Query Chalet for project-specific information. Intended for IDE integrations." },
		{ Route::ColorTest, "Display all color themes and terminal capabilities." },
	}),
	m_routeMap({
		{ "buildrun", Route::BuildRun },
		{ "run", Route::Run },
		{ "build", Route::Build },
		{ "rebuild", Route::Rebuild },
		{ "clean", Route::Clean },
		{ "bundle", Route::Bundle },
		{ "configure", Route::Configure },
		{ "export", Route::Export },
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
	m_subCommands.emplace(Route::Debug, &ArgumentParser::populateDebugArguments);
	m_routeMap.emplace("debug", Route::Debug);
#endif
}

/*****************************************************************************/
StringList ArgumentParser::getTruthyArguments() const
{
	return {
		"--show-commands",
		"--dump-assembly",
		"--benchmark",
		"--launch-profiler",
		"--keep-going",
		"--generate-compile-commands",
		"--save-schema",
		"--quieter",
		"-l",
		"--local",
		"-g",
		"--global",
	};
}

/*****************************************************************************/
bool ArgumentParser::resolveFromArguments(const int argc, const char* argv[])
{
	int maxPositionalArgs = 2;
	if (!parse(argc, argv, maxPositionalArgs))
	{
		Diagnostic::fatalError("Bad argument parse");
		return false;
	}

	/*for (auto&& [key, value] : m_rawArguments)
	{
		if (value.empty())
			LOG(key);
		else
			LOG(key, "=", value);
	}
	LOG("");*/

	m_argumentList.clear();

	if (m_rawArguments.find(Positional::Argument1) != m_rawArguments.end())
	{
		m_routeString = m_rawArguments.at(Positional::Argument1);
		m_route = getRouteFromString(m_routeString);
		if (m_subCommands.find(m_route) != m_subCommands.end())
		{
			makeParser();

			auto& command = m_subCommands.at(m_route);
			command(*this);
			return doParse();
		}
	}

	m_route = Route::Unknown;
	m_routeString.clear();
	makeParser();

	populateMainArguments();

	return doParse();
}

/*****************************************************************************/
Route ArgumentParser::getRouteFromString(const std::string& inValue)
{
	if (m_routeMap.find(inValue) != m_routeMap.end())
		return m_routeMap.at(inValue);

	return Route::Unknown;
}

/*****************************************************************************/
const ArgumentParser::ArgumentList& ArgumentParser::arguments() const noexcept
{
	return m_argumentList;
}

Route ArgumentParser::route() const noexcept
{
	return m_route;
}

/*****************************************************************************/
StringList ArgumentParser::getRouteList()
{
	StringList ret;
	for (auto& [cmd, _] : m_routeMap)
	{
		ret.emplace_back(cmd);
	}

	return ret;
}

/*****************************************************************************/
std::string ArgumentParser::getProgramPath() const
{
	chalet_assert(!m_rawArguments.empty(), "!m_rawArguments.empty()");
	if (m_rawArguments.find(Positional::ProgramArgument) != m_rawArguments.end())
		return m_rawArguments.at(Positional::ProgramArgument);

	return std::string();
}

/*****************************************************************************/
void ArgumentParser::makeParser()
{
	const std::string program = "chalet";

	addHelpArg();
	addVersionArg();

	if (m_route != Route::Unknown && !m_routeString.empty())
	{
		m_argumentList.emplace_back(ArgumentIdentifier::RouteString, true)
			.addArgument(Positional::Argument1, m_routeString)
			.setHelp("This subcommand.")
			.setRequired();
	}
}

/*****************************************************************************/
bool ArgumentParser::doParse()
{
	if (containsOption("-h", "--help") || m_rawArguments.size() == 1)
	{
		return showHelp();
	}
	else if (containsOption("-v", "--version"))
	{
		return showVersion();
	}
	else
	{
		if (m_routeString.empty())
		{
			if (containsOption(Positional::Argument1))
				Diagnostic::fatalError("Invalid subcommand requested: '{}'. See 'chalet --help'.", m_rawArguments.at(Positional::Argument1));
			else
				Diagnostic::fatalError("Invalid argument(s) found. See 'chalet --help'.");
			return false;
		}

		if (!assignArgumentListFromArgumentsAndValidate())
			return false;

		return true;
	}
}

/*****************************************************************************/
bool ArgumentParser::showHelp()
{
	std::string help = getHelp();
	std::cout.write(help.data(), help.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();

	m_route = Route::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentParser::showVersion()
{
	std::string version = "Chalet version 0.3.3";
	std::cout.write(version.data(), version.size());
	std::cout.put(std::cout.widen('\n'));
	std::cout.flush();

	m_route = Route::Help;
	return true;
}

/*****************************************************************************/
std::string ArgumentParser::getSeeHelpMessage()
{
	if (!m_routeString.empty())
		return fmt::format("See 'chalet {} --help'.", m_routeString);
	else
		return std::string("See 'chalet --help'.");
}

/*****************************************************************************/
bool ArgumentParser::assignArgumentListFromArgumentsAndValidate()
{
	StringList invalid;
	for (auto& [key, _] : m_rawArguments)
	{
		if (String::equals(Positional::RemainingArguments, key) || String::startsWith('@', key))
			continue;

		bool valid = false;
		for (auto& mapped : m_argumentList)
		{
			if (String::equals(key, mapped.key()) || String::equals(key, mapped.keyLong()))
			{
				valid = true;
				break;
			}
		}
		if (!valid)
			invalid.push_back(key);
	}

	if (!invalid.empty())
	{
		auto seeHelp = getSeeHelpMessage();
		Diagnostic::fatalError("Unknown argument: '{}'. {}", invalid.front(), seeHelp);
		return false;
	}

	m_hasRemaining = containsOption(Positional::RemainingArguments);
	bool allowsRemaining = false;

	int maxPositionalArgs = 0;

	// TODO: Check invalid
	for (auto& mapped : m_argumentList)
	{
		if (String::startsWith('@', mapped.key()))
			++maxPositionalArgs;

		if (mapped.id() == ArgumentIdentifier::RouteString)
			continue;

		allowsRemaining |= String::equals(Positional::RemainingArguments, mapped.key());

		std::string value;
		if (containsOption(mapped.key()))
		{
			value = m_rawArguments.at(mapped.key());
		}
		else if (containsOption(mapped.keyLong()))
		{
			value = m_rawArguments.at(mapped.keyLong());
		}
		else if (mapped.required())
		{
			auto seeHelp = getSeeHelpMessage();
			Diagnostic::fatalError("Missing required argument: '{}'. {}", mapped.keyLong(), seeHelp);
			return false;
		}

		if (value.empty())
			continue;

		switch (mapped.value().kind())
		{
			case Variant::Kind::Boolean:
				mapped.setValue(String::equals("1", value));
				break;

			case Variant::Kind::OptionalBoolean: {
				if (!value.empty())
				{
					int rawValue = atoi(value.c_str());
					mapped.setValue(std::optional<bool>(rawValue == 1));
				}
				break;
			}

			case Variant::Kind::Integer:
				mapped.setValue<int>(atoi(value.c_str()));
				break;

			case Variant::Kind::OptionalInteger: {
				if (!value.empty())
				{
					mapped.setValue(std::optional<int>(atoi(value.c_str())));
				}
				break;
			}

			case Variant::Kind::String:
				mapped.setValue(value);
				break;

			case Variant::Kind::Empty:
			case Variant::Kind::StringList:
			default:
				break;
		}
	}

	int positionalArgs = 0;
	for (auto& [key, _] : m_rawArguments)
	{
		if (String::equals(Positional::ProgramArgument, key))
			continue;

		if (String::startsWith('@', key))
			++positionalArgs;
	}

	if (positionalArgs > maxPositionalArgs)
	{
		auto seeHelp = getSeeHelpMessage();
		Diagnostic::fatalError("Maximum number of positional arguments exceeded. {}", seeHelp);
		return false;
	}

	if (m_hasRemaining && !allowsRemaining)
	{
		auto seeHelp = getSeeHelpMessage();
		auto& remaining = m_rawArguments.at(Positional::RemainingArguments);
		Diagnostic::fatalError("Maximum number of positional arguments exceeded, starting with: '{}'. {}", remaining, seeHelp);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ArgumentParser::getHelp()
{
	constexpr std::size_t kColumnSize = 28;

	std::string title = "Chalet - A cross-platform JSON-based project & build tool";

	std::string help;
	help += title + '\n';
	help += '\n';
	help += "Usage:\n";
	std::string command("chalet");
	for (auto& mapped : m_argumentList)
	{
		const auto id = mapped.id();
		if (id == ArgumentIdentifier::SubCommand)
		{
			command += ' ';
			command += mapped.key();
			command += " [options]";
		}
		else if (id == ArgumentIdentifier::RouteString)
		{
			command += ' ';
			command += mapped.keyLong();
			command += " [options]";
		}
		else if (!String::startsWith('-', mapped.key()))
		{
			command += ' ';
			command += mapped.keyLong();
		}
	}
	help += fmt::format("   {}\n", command);
	help += '\n';
	if (m_route != Route::Unknown)
	{
		help += "Description:\n";
		help += fmt::format("   {}\n", m_routeDescriptions.at(m_route));
		help += '\n';
	}
	help += "Commands:\n";

	for (auto& mapped : m_argumentList)
	{
		const auto id = mapped.id();
		if (id == ArgumentIdentifier::SubCommand)
		{
			help += fmt::format("{}\n", mapped.help());
		}
		else if (!String::startsWith('-', mapped.key()))
		{
			std::string line = mapped.keyLong();
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += mapped.help();

			help += fmt::format("{}\n", line);
		}
	}

	help += '\n';
	help += "Options:\n";

	for (auto& mapped : m_argumentList)
	{
		if (String::startsWith('-', mapped.key()))
		{
			std::string line = mapped.key() + ' ' + mapped.keyLong();
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += mapped.help();

			help += fmt::format("{}\n", line);
		}
	}

	if (String::contains("--toolchain", help))
	{
		auto getPresetMessage = [](const std::string& preset) -> std::string {
			if (String::equals("llvm", preset))
				return std::string("The LLVM Project");
#if defined(CHALET_WIN32)
			else if (String::equals("gcc", preset))
				return std::string("MinGW: Minimalist GNU Compiler Collection for Windows");
#else
			else if (String::equals("gcc", preset))
				return std::string("GNU Compiler Collection");
#endif
#if defined(CHALET_MACOS)
			else if (String::equals("apple-llvm", preset))
				return fmt::format("Apple{} LLVM (Requires Xcode or \"Command Line Tools for Xcode\")", Unicode::registered());
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICC
			else if (String::equals("intel-classic", preset))
				return fmt::format("Intel{} C++ Compiler Classic (for x86_64 processors)", Unicode::registered());
	#endif
#elif defined(CHALET_WIN32)
			else if (String::equals("vs-stable", preset))
				return fmt::format("Microsoft{} Visual Studio (latest installed stable release)", Unicode::registered());
			else if (String::equals("vs-preview", preset))
				return fmt::format("Microsoft{} Visual Studio (latest installed preview release)", Unicode::registered());
			else if (String::equals("vs-2022", preset))
				return fmt::format("Microsoft{} Visual Studio 2022", Unicode::registered());
			else if (String::equals("vs-2019", preset))
				return fmt::format("Microsoft{} Visual Studio 2019", Unicode::registered());
			else if (String::equals("vs-2017", preset))
				return fmt::format("Microsoft{} Visual Studio 2017", Unicode::registered());
	#if CHALET_EXPERIMENTAL_ENABLE_INTEL_ICX
			else if (String::equals("intel-llvm-vs-2022", preset))
				return fmt::format("Intel{} oneAPI DPC++/C++ Compiler with Visual Studio 2022 environment", Unicode::registered());
			else if (String::equals("intel-llvm-vs-2019", preset))
				return fmt::format("Intel{} oneAPI DPC++/C++ Compiler with Visual Studio 2019 environment", Unicode::registered());
	#endif
#endif

			return std::string();
		};

		const auto& defaultToolchain = m_inputs.defaultToolchainPreset();

		help += "\nToolchain Presets:\n";
		const auto toolchains = m_inputs.getToolchainPresets();
		for (auto& toolchain : toolchains)
		{
			const bool isDefault = String::equals(defaultToolchain, toolchain);
			std::string line = toolchain;
			while (line.size() < kColumnSize)
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
MappedArgument& ArgumentParser::addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::String)
		.addArgument(inShort, inLong)
		.setValue(std::move(inDefaultValue));
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::OptionalInteger)
		.addArgument(inShort, inLong);
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::Boolean)
		.addArgument(inArgument)
		.setValue(inDefaultValue);
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::OptionalBoolean)
		.addArgument(inArgument);
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::Boolean)
		.addArgument(inShort, inLong)
		.setValue(inDefaultValue);
}

/*****************************************************************************/
void ArgumentParser::populateMainArguments()
{
	StringList subcommands;
	StringList descriptions;

	subcommands.push_back(fmt::format("init [{}]", Arg::InitPath));
	descriptions.push_back(m_routeDescriptions.at(Route::Init));

	subcommands.push_back(fmt::format("export [{}]", Arg::ExportKind));
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(Route::Export)));

	subcommands.push_back("configure");
	descriptions.push_back(m_routeDescriptions.at(Route::Configure));

	subcommands.push_back(fmt::format("buildrun {} {}", Arg::RunTarget, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(Route::BuildRun));

	subcommands.push_back(fmt::format("run {} {}", Arg::RunTarget, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(Route::Run));

	subcommands.push_back("build");
	descriptions.push_back(m_routeDescriptions.at(Route::Build));

	subcommands.push_back("rebuild");
	descriptions.push_back(m_routeDescriptions.at(Route::Rebuild));

	subcommands.push_back("clean");
	descriptions.push_back(m_routeDescriptions.at(Route::Clean));

	subcommands.push_back("bundle");
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(Route::Bundle)));

	subcommands.push_back(fmt::format("get {}", Arg::SettingsKey));
	descriptions.push_back(m_routeDescriptions.at(Route::SettingsGet));

	subcommands.push_back(fmt::format("getkeys {}", Arg::SettingsKeyQuery));
	descriptions.push_back(m_routeDescriptions.at(Route::SettingsGetKeys));

	subcommands.push_back(fmt::format("set {} {}", Arg::SettingsKey, Arg::SettingsValue));
	descriptions.push_back(m_routeDescriptions.at(Route::SettingsSet));

	subcommands.push_back(fmt::format("unset {}", Arg::SettingsKey));
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(Route::SettingsUnset)));

	subcommands.push_back(fmt::format("query {} {}", Arg::QueryType, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(Route::Query));

	subcommands.push_back("colortest");
	descriptions.push_back(m_routeDescriptions.at(Route::ColorTest));

	std::string help;
	if (subcommands.size() == descriptions.size())
	{
		for (std::size_t i = 0; i < subcommands.size(); ++i)
		{
			std::string line = subcommands.at(i);
			while (line.size() < 28)
				line += ' ';
			line += '\t';
			line += descriptions.at(i);

			help += line;
			if (i < (subcommands.size() - 1))
				help += '\n';
		}
	}
	else
	{
		chalet_assert(false, "vector size mismatch between subcommands and descriptions");
		help += "Error!";
	}

	addBoolArgument(ArgumentIdentifier::SubCommand, "<subcommand>", true)
		.setHelp(std::move(help));
}

/*****************************************************************************/
void ArgumentParser::addHelpArg()
{
	addTwoBoolArguments(ArgumentIdentifier::Help, "-h", "--help", false)
		.setHelp("Shows help message (if applicable, for the subcommand) and exits.");
}

/*****************************************************************************/
void ArgumentParser::addVersionArg()
{
	addTwoBoolArguments(ArgumentIdentifier::Version, "-v", "--version", false)
		.setHelp("Prints version information and exits.");
}

/*****************************************************************************/
void ArgumentParser::addInputFileArg()
{
	const auto& defaultValue = m_inputs.defaultInputFile();
	addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file")
		.setHelp(fmt::format("An input build file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addSettingsFileArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	addTwoStringArguments(ArgumentIdentifier::SettingsFile, "-s", "--settings-file")
		.setHelp(fmt::format("The path to a settings file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addFileArg()
{
	addTwoStringArguments(ArgumentIdentifier::File, "-f", "--file")
		.setHelp("The path to a JSON file to examine, if not the local/global settings.");
}

/*****************************************************************************/
void ArgumentParser::addRootDirArg()
{
	addTwoStringArguments(ArgumentIdentifier::RootDirectory, "-r", "--root-dir")
		.setHelp("The root directory to run the build from. [default: \".\"]");
}

/*****************************************************************************/
void ArgumentParser::addOutputDirArg()
{
	const auto& defaultValue = m_inputs.defaultOutputDirectory();
	addTwoStringArguments(ArgumentIdentifier::OutputDirectory, "-o", "--output-dir")
		.setHelp(fmt::format("The output directory of the build. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addExternalDirArg()
{
	const auto& defaultValue = m_inputs.defaultExternalDirectory();
	addTwoStringArguments(ArgumentIdentifier::ExternalDirectory, "-x", "--external-dir")
		.setHelp(fmt::format("The directory to install external dependencies into. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addDistributionDirArg()
{
	const auto& defaultValue = m_inputs.defaultDistributionDirectory();
	addTwoStringArguments(ArgumentIdentifier::DistributionDirectory, "-d", "--distribution-dir")
		.setHelp(fmt::format("The root directory for all distribution bundles. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addProjectGenArg()
{
// future
#if 0
	addTwoStringArguments("-p", "--project-gen")
		.help("Project file generator [vs2019,vscode,xcode]");
#endif
}

/*****************************************************************************/
void ArgumentParser::addToolchainArg()
{
	const auto& defaultValue = m_inputs.defaultToolchainPreset();
	addTwoStringArguments(ArgumentIdentifier::Toolchain, "-t", "--toolchain")
		.setHelp(fmt::format("A toolchain or toolchain preset to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addMaxJobsArg()
{
	auto jobs = std::thread::hardware_concurrency();
	addTwoIntArguments(ArgumentIdentifier::MaxJobs, "-j", "--max-jobs")
		.setHelp(fmt::format("The number of jobs to run during compilation. [default: {}]", jobs));
}

/*****************************************************************************/
void ArgumentParser::addEnvFileArg()
{
	const auto& defaultValue = m_inputs.defaultEnvFile();
	addTwoStringArguments(ArgumentIdentifier::EnvFile, "-e", "--env-file")
		.setHelp(fmt::format("A file to load environment variables from. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addArchArg()
{
	addTwoStringArguments(ArgumentIdentifier::TargetArchitecture, "-a", "--arch", "auto")
		.setHelp("The architecture to target for the build.");
}

/*****************************************************************************/
void ArgumentParser::addSaveSchemaArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::SaveSchema, "--save-schema")
		.setHelp("Save build & settings schemas to file.");
}

/*****************************************************************************/
void ArgumentParser::addQuietArgs()
{
	// TODO: other quiet flags
	// --quiet = build & initialization output (no descriptions or flare)
	// --quieter = just build output
	// --quietest = no output

	addOptionalBoolArgument(ArgumentIdentifier::Quieter, "--quieter")
		.setHelp("Show only the build output.");
}

/*****************************************************************************/
void ArgumentParser::addBuildConfigurationArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildConfiguration, "-c", "--configuration")
		.setHelp("The build configuration to use. [default: \"Release\"]");
}

/*****************************************************************************/
void ArgumentParser::addRunTargetArg()
{
	addTwoStringArguments(ArgumentIdentifier::RunTargetName, Positional::Argument2, Arg::RunTarget)
		.setHelp("An executable or script target to run.");
}

/*****************************************************************************/
void ArgumentParser::addRunArgumentsArg()
{
	addTwoStringArguments(ArgumentIdentifier::RunTargetArguments, Positional::RemainingArguments, Arg::RemainingArguments)
		.setHelp("The arguments to pass to the run target.");
}

/*****************************************************************************/
void ArgumentParser::addSettingsTypeArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	addTwoBoolArguments(ArgumentIdentifier::LocalSettings, "-l", "--local", false)
		.setHelp(fmt::format("Use the local settings. [{}]", defaultValue));

	const auto& globalSettings = m_inputs.globalSettingsFile();
	addTwoBoolArguments(ArgumentIdentifier::GlobalSettings, "-g", "--global", false)
		.setHelp(fmt::format("Use the global settings. [~/{}]", globalSettings));
}

/*****************************************************************************/
void ArgumentParser::addDumpAssemblyArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::DumpAssembly, "--dump-assembly")
		.setHelp("Create an .asm dump of each object file during the build.");
}

/*****************************************************************************/
void ArgumentParser::addGenerateCompileCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::GenerateCompileCommands, "--generate-compile-commands")
		.setHelp("Generate a compile_commands.json file for Clang tooling use.");
}

/*****************************************************************************/
void ArgumentParser::addShowCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::ShowCommands, "--show-commands")
		.setHelp("Show the commands run during the build.");
}

/*****************************************************************************/
void ArgumentParser::addBenchmarkArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::Benchmark, "--benchmark")
		.setHelp("Show all build times - total build time, build targets, other steps.");
}

/*****************************************************************************/
void ArgumentParser::addLaunchProfilerArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::LaunchProfiler, "--launch-profiler")
		.setHelp("If running profile targets, launch the preferred profiler afterwards.");
}

/*****************************************************************************/
void ArgumentParser::addKeepGoingArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::KeepGoing, "--keep-going")
		.setHelp("If there's a build error, continue as much of the build as possible.");
}

/*****************************************************************************/
void ArgumentParser::populateBuildRunArguments()
{
	populateBuildArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentParser::populateRunArguments()
{
	populateBuildArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentParser::populateBuildArguments()
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
void ArgumentParser::populateInitArguments()
{
	const auto templates = m_inputs.getProjectInitializationPresets();
	addTwoStringArguments(ArgumentIdentifier::InitTemplate, "-t", "--template")
		.setHelp(fmt::format("The project template to use during initialization. (ex: {})", String::join(templates, ", ")));

	addTwoStringArguments(ArgumentIdentifier::InitPath, Positional::Argument2, Arg::InitPath, ".")
		.setHelp("The path of the project to initialize. [default: \".\"]");
}

/*****************************************************************************/
void ArgumentParser::populateExportArguments()
{
	addToolchainArg();
	addArchArg();

	const auto kinds = m_inputs.getExportKindPresets();
	addTwoStringArguments(ArgumentIdentifier::ExportKind, Positional::Argument2, Arg::ExportKind)
		.setHelp(fmt::format("The project kind to export to. (ex: {})", String::join(kinds, ", ")))
		.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateSettingsGetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey)
		.setHelp("The config key to get.");
}

/*****************************************************************************/
void ArgumentParser::populateSettingsGetKeysArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKeyQuery)
		.setHelp("The config key to query for.");

	addTwoStringArguments(ArgumentIdentifier::SettingsKeysRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments)
		.setHelp("Additional query arguments, if applicable.");
}

/*****************************************************************************/
void ArgumentParser::populateSettingsSetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey)
		.setHelp("The config key to change.")
		.setRequired();

	addTwoStringArguments(ArgumentIdentifier::SettingsValue, Positional::RemainingArguments, Arg::SettingsValue)
		.setHelp("The config value to change to.")
		.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateSettingsUnsetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey)
		.setHelp("The config key to remove.")
		.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateQueryArguments()
{
	auto listNames = m_inputs.getCliQueryOptions();
	addTwoStringArguments(ArgumentIdentifier::QueryType, Positional::Argument2, Arg::QueryType)
		.setHelp(fmt::format("The data type to query. ({})", String::join(listNames, ", ")))
		.setRequired();

	addTwoStringArguments(ArgumentIdentifier::QueryDataRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments)
		.setHelp("Data to provide to the query. (architecture: <toolchain-name>)");
}

/*****************************************************************************/
void ArgumentParser::populateColorTestArguments()
{
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentParser::populateDebugArguments()
{
	populateBuildArguments();
}
#endif
}
