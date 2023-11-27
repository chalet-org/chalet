/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Core/Arguments/ArgumentParser.hpp"

#include <thread>

#include "Core/CommandLineInputs.hpp"
#include "Core/Router/RouteType.hpp"
#include "State/CompilerTools.hpp"
#include "System/DefinesVersion.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/List.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"

namespace chalet
{
namespace Arg
{
CHALET_CONSTANT(BuildTarget) = "[<target>]";
CHALET_CONSTANT(RemainingArguments) = "[ARG...]";
// CHALET_CONSTANT(InitName) = "<name>";
CHALET_CONSTANT(InitPath) = "<path>";
CHALET_CONSTANT(ExportKind) = "<kind>";
CHALET_CONSTANT(SettingsKey) = "<key>";
CHALET_CONSTANT(SettingsKeyQuery) = "<query>";
CHALET_CONSTANT(SettingsValue) = "<value>";
CHALET_CONSTANT(ValidateSchema) = "<schema>";
CHALET_CONSTANT(QueryType) = "<type>";
// CHALET_CONSTANT(QueryData) = "<data>";
CHALET_CONSTANT(ConvertFormat) = "<format>";
}

namespace Positional
{
CHALET_CONSTANT(ProgramArgument) = "@0";
CHALET_CONSTANT(Argument1) = "@1";
CHALET_CONSTANT(Argument2) = "@2";
CHALET_CONSTANT(RemainingArguments) = "...";
}

namespace
{
constexpr size_t kColumnSize = 32;
}

/*****************************************************************************/
ArgumentParser::ArgumentParser(const CommandLineInputs& inInputs) :
	BaseArgumentParser(),
	m_inputs(inInputs),
	m_subCommands({
		{ RouteType::BuildRun, &ArgumentParser::populateBuildRunArguments },
		{ RouteType::Run, &ArgumentParser::populateRunArguments },
		{ RouteType::Build, &ArgumentParser::populateBuildArguments },
		{ RouteType::Rebuild, &ArgumentParser::populateBuildArguments },
		{ RouteType::Clean, &ArgumentParser::populateCommonBuildArguments },
		{ RouteType::Bundle, &ArgumentParser::populateCommonBuildArguments },
		{ RouteType::Configure, &ArgumentParser::populateCommonBuildArguments },
		{ RouteType::Init, &ArgumentParser::populateInitArguments },
		{ RouteType::Export, &ArgumentParser::populateExportArguments },
		{ RouteType::SettingsGet, &ArgumentParser::populateSettingsGetArguments },
		{ RouteType::SettingsGetKeys, &ArgumentParser::populateSettingsGetKeysArguments },
		{ RouteType::SettingsSet, &ArgumentParser::populateSettingsSetArguments },
		{ RouteType::SettingsUnset, &ArgumentParser::populateSettingsUnsetArguments },
		{ RouteType::Validate, &ArgumentParser::populateValidateArguments },
		{ RouteType::Query, &ArgumentParser::populateQueryArguments },
		{ RouteType::Convert, &ArgumentParser::populateConvertArguments },
		{ RouteType::TerminalTest, &ArgumentParser::populateTerminalTestArguments },
	}),
	m_routeDescriptions({
		{ RouteType::BuildRun, "Build a project and run a valid executable build target." },
		{ RouteType::Run, "Run a valid executable build target." },
		{ RouteType::Build, "Build a project and create its configuration if it doesn't exist." },
		{ RouteType::Rebuild, "Rebuild the project and create a configuration if it doesn't exist." },
		{ RouteType::Clean, "Unceremoniously clean the build folder." },
		{ RouteType::Bundle, "Bundle a project for distribution." },
		{ RouteType::Configure, "Create a project configuration and fetch external dependencies." },
		{ RouteType::Export, "Export the project to another project format." },
		{ RouteType::Init, "Initialize a project in either the current directory or a subdirectory." },
		{ RouteType::SettingsGet, "If the given property is valid, display its JSON node." },
		{ RouteType::SettingsGetKeys, "If the given property is an object, display the names of its properties." },
		{ RouteType::SettingsSet, "Set the given property to the given value." },
		{ RouteType::SettingsUnset, "Remove the key/value pair given a valid property key." },
		{ RouteType::Validate, "Validate JSON file(s) against a schema." },
		{ RouteType::Query, "Query Chalet for project-specific information. Intended for IDE integrations." },
		{ RouteType::Convert, "Convert the build file from one supported format to another." },
		{ RouteType::TerminalTest, "Display all color themes and terminal capabilities." },
	}),
	m_routeMap({
		{ "buildrun", RouteType::BuildRun },
		{ "r", RouteType::BuildRun },
		{ "run", RouteType::Run },
		{ "build", RouteType::Build },
		{ "b", RouteType::Build },
		{ "rebuild", RouteType::Rebuild },
		{ "clean", RouteType::Clean },
		{ "bundle", RouteType::Bundle },
		{ "configure", RouteType::Configure },
		{ "c", RouteType::Configure },
		{ "export", RouteType::Export },
		{ "init", RouteType::Init },
		{ "get", RouteType::SettingsGet },
		{ "getkeys", RouteType::SettingsGetKeys },
		{ "set", RouteType::SettingsSet },
		{ "unset", RouteType::SettingsUnset },
		{ "validate", RouteType::Validate },
		{ "query", RouteType::Query },
		{ "convert", RouteType::Convert },
		{ "termtest", RouteType::TerminalTest },
	})
{
#if defined(CHALET_DEBUG)
	m_subCommands.emplace(RouteType::Debug, &ArgumentParser::populateDebugArguments);
	m_routeMap.emplace("debug", RouteType::Debug);
#endif
}

/*****************************************************************************/
StringList ArgumentParser::getTruthyArguments() const
{
	return {
		"--show-commands",
		"--no-show-commands",
		"--dump-assembly",
		"--no-dump-assembly",
		"--benchmark",
		"--no-benchmark",
		"--launch-profiler",
		"--no-launch-profiler",
		"--keep-going",
		"--no-keep-going",
		"--generate-compile-commands",
		"--no-generate-compile-commands",
		"--only-required",
		"--no-only-required",
		"--save-user-toolchain-globally",
		"--save-schema",
		"--quieter",
		"-l",
		"--local",
		"-g",
		"--global",
	};
}

/*****************************************************************************/
bool ArgumentParser::resolveFromArguments(const i32 argc, const char* argv[])
{
	i32 maxPositionalArgs = 2;
	if (!parse(argc, argv, maxPositionalArgs))
	{
		Diagnostic::error("Bad argument parse");
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

	m_route = RouteType::Unknown;
	m_routeString.clear();
	makeParser();

	populateMainArguments();

	return doParse();
}

/*****************************************************************************/
RouteType ArgumentParser::getRouteFromString(const std::string& inValue) const
{
	if (m_routeMap.find(inValue) != m_routeMap.end())
		return m_routeMap.at(inValue);

	return RouteType::Unknown;
}

/*****************************************************************************/
const ArgumentParser::ArgumentList& ArgumentParser::arguments() const noexcept
{
	return m_argumentList;
}

CommandRoute ArgumentParser::getRoute() const noexcept
{
	CommandRoute ret(m_route);
	return ret;
}

/*****************************************************************************/
StringList ArgumentParser::getRouteList() const
{
	StringList exclude{ "b", "r", "c" };
	StringList ret;
	for (auto& [cmd, _] : m_routeMap)
	{
		if (String::equals(exclude, cmd))
			continue;

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
StringList ArgumentParser::getAllCliOptions()
{
	StringList ret;

	ArgumentList previousArgumentList = std::move(m_argumentList);

	// if (m_argumentList.empty())
	{
		addHelpArg();
		addVersionArg();
		populateCommonBuildArguments();
		addSettingsTypeArg();
		ret.emplace_back("--template");
	}

	for (const auto& arg : m_argumentList)
	{
		const auto& key = arg.key();
		const auto& keyLong = arg.keyLong();

		if (!key.empty() && String::startsWith('-', key))
			ret.emplace_back(key);

		if (!keyLong.empty() && String::startsWith('-', keyLong))
			ret.emplace_back(keyLong);
	}

	m_argumentList = std::move(previousArgumentList);

	std::sort(ret.begin(), ret.end());

	return ret;
}

/*****************************************************************************/
void ArgumentParser::makeParser()
{
	addHelpArg();

	if (isSubcommand())
	{
		auto routeString = m_routeString;
		if (String::equals({ "buildrun", "r" }, routeString))
			routeString = "buildrun,r";
		else if (String::equals({ "build", "b" }, routeString))
			routeString = "build,b";
		else if (String::equals({ "configure", "c" }, routeString))
			routeString = "configure,c";

		m_argumentList.emplace_back(ArgumentIdentifier::RouteString, true)
			.addArgument(Positional::Argument1, routeString)
			.setHelp("This subcommand.")
			.setRequired();
	}
	else
	{
		addVersionArg();
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
		if (isSubcommand())
			return showHelp();
		else
			return showVersion();
	}
	else
	{
		if (m_routeString.empty())
		{
			if (containsOption(Positional::Argument1))
				Diagnostic::error("Invalid subcommand: '{}'. See 'chalet --help'.", m_rawArguments.at(Positional::Argument1));
			else
				Diagnostic::error("Invalid argument(s) found. See 'chalet --help'.");
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
	std::cout.put('\n');
	std::cout.flush();

	m_route = RouteType::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentParser::showVersion()
{
	std::string version = fmt::format("Chalet version {}", CHALET_VERSION);
	std::cout.write(version.data(), version.size());
	std::cout.put('\n');
	std::cout.flush();

	m_route = RouteType::Help;
	return true;
}

/*****************************************************************************/
bool ArgumentParser::isSubcommand() const
{
	return m_route != RouteType::Unknown && !m_routeString.empty();
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
	// Add exceptions for routes with only required remaining arguments
	// we need to remove the assumed @2 for the first of them
	// bool routeHasOnePositionalArgument = m_route == RouteType::Validate;
	// if (routeHasOnePositionalArgument)
	// {
	// 	// If it doesn't exist, let it fail later
	// 	if (containsOption(Positional::Argument2))
	// 	{
	// 		auto value = m_rawArguments.at(Positional::Argument2);
	// 		m_rawArguments.erase(Positional::Argument2);
	// 		m_rawArguments[Positional::RemainingArguments] = fmt::format("'{}' {}", value, m_rawArguments[Positional::RemainingArguments]);
	// 	}
	// }

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
		Diagnostic::error("Unknown argument: '{}'. {}", invalid.front(), seeHelp);
		return false;
	}

	m_hasRemaining = containsOption(Positional::RemainingArguments);

	bool allowsRemaining = false;

	i32 maxPositionalArgs = 0;

	// TODO: Check invalid
	for (auto& mapped : m_argumentList)
	{
		bool isRemaining = String::equals(Positional::RemainingArguments, mapped.key());
		if (String::startsWith('@', mapped.key()))
			maxPositionalArgs++;

		if (mapped.id() == ArgumentIdentifier::RouteString)
			continue;

		allowsRemaining |= isRemaining;

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
			Diagnostic::error("Missing required argument: '{}'. {}", mapped.keyLong(), seeHelp);
			return false;
		}

		if (value.empty())
			continue;

		switch (mapped.value().kind())
		{
			case Variant::Kind::Boolean:
				mapped.setValue(String::equals('1', value));
				break;

			case Variant::Kind::OptionalBoolean: {
				mapped.setValue(std::optional<bool>(String::equals('1', value)));
				break;
			}

			case Variant::Kind::Integer:
				mapped.setValue<i32>(atoi(value.c_str()));
				break;

			case Variant::Kind::OptionalInteger: {
				if (!value.empty())
				{
					mapped.setValue(std::optional<i32>(atoi(value.c_str())));
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

	i32 positionalArgs = 0;
	for (auto& [key, _] : m_rawArguments)
	{
		if (String::equals(Positional::ProgramArgument, key))
			continue;

		if (key.empty())
			continue;

		if (String::startsWith('@', key))
			positionalArgs++;
	}

	if (positionalArgs > maxPositionalArgs)
	{
		auto seeHelp = getSeeHelpMessage();
		Diagnostic::error("Maximum number of positional arguments exceeded. {}", seeHelp);
		return false;
	}

	if (m_hasRemaining && !allowsRemaining)
	{
		auto seeHelp = getSeeHelpMessage();
		auto& remaining = m_rawArguments.at(Positional::RemainingArguments);
		Diagnostic::error("Maximum number of positional arguments exceeded, starting with: '{}'. {}", remaining, seeHelp);
		return false;
	}

	return true;
}

/*****************************************************************************/
std::string ArgumentParser::getHelp()
{
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
	if (m_route != RouteType::Unknown)
	{
		help += "Description:\n";
		help += fmt::format("{}\n", m_routeDescriptions.at(m_route));
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
			std::string line;
			if (mapped.keyLabel().empty())
				line = mapped.key() + ' ' + mapped.keyLong();
			else
				line = mapped.keyLabel();

			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += mapped.help();

			help += fmt::format("{}\n", line);
		}
	}

	if (String::contains("--toolchain", help))
	{
		auto getToolchainPresetDescription = [](const std::string& preset) -> std::string {
			if (String::equals("llvm", preset))
				return std::string("The LLVM Compiler Infrastructure Project");
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
	#if CHALET_ENABLE_INTEL_ICC
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
			else if (String::equals("llvm-vs-stable", preset))
				return fmt::format("LLVM/Clang in Microsoft{} Visual Studio (latest stable)", Unicode::registered());
			else if (String::equals("llvm-vs-preview", preset))
				return fmt::format("LLVM/CLang in Microsoft{} Visual Studio (latest preview)", Unicode::registered());
			else if (String::equals("llvm-vs-2022", preset))
				return fmt::format("LLVM/Clang in Microsoft{} Visual Studio 2022", Unicode::registered());
			else if (String::equals("llvm-vs-2019", preset))
				return fmt::format("LLVM/Clang in Microsoft{} Visual Studio 2019", Unicode::registered());
	#if CHALET_ENABLE_INTEL_ICX
			else if (String::equals("intel-llvm-vs-2022", preset))
				return fmt::format("Intel{} oneAPI DPC++/C++ Compiler with Visual Studio 2022 environment", Unicode::registered());
			else if (String::equals("intel-llvm-vs-2019", preset))
				return fmt::format("Intel{} oneAPI DPC++/C++ Compiler with Visual Studio 2019 environment", Unicode::registered());
	#endif
#endif

			return std::string();
		};

		const auto& defaultToolchain = m_inputs.defaultToolchainPreset();

		help += "\nToolchain presets:\n";
		const auto toolchains = m_inputs.getToolchainPresets();
		for (auto& toolchain : toolchains)
		{
			const bool isDefault = String::equals(defaultToolchain, toolchain);
			std::string line = toolchain;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getToolchainPresetDescription(toolchain);
			if (isDefault)
				line += " [default]";

			help += fmt::format("{}\n", line);
		}
	}

	if (String::contains("--build-strategy", help))
	{
		auto getStrategyPresetDescription = [](const std::string& preset) -> std::string {
			if (String::equals("ninja", preset))
				return std::string("Build with Ninja");
			else if (String::equals("makefile", preset))
#if defined(CHALET_WIN32)
				return std::string("Build with GNU Make (MinGW), NMake or Qt Jom (MSVC)");
#else
				return std::string("Build with GNU Make");
#endif
			else if (String::equals("native", preset))
				return std::string("Build natively with Chalet");
#if defined(CHALET_WIN32)
			else if (String::equals("msbuild", preset))
				return std::string("Build using a Visual Studio solution and MSBuild - requires vs-* toolchain preset");
#elif defined(CHALET_MACOS)
			else if (String::equals("xcodebuild", preset))
				return std::string("Build using an Xcode project and xcodebuild - requires apple-llvm toolchain preset");
#endif

			return std::string();
		};

		help += "\nBuild strategies:\n";
		StringList strategies = CompilerTools::getToolchainStrategies();

		for (auto& strat : strategies)
		{
			std::string line = strat;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getStrategyPresetDescription(strat);

			help += fmt::format("{}\n", line);
		}
	}

	if (String::contains("--build-path-style", help))
	{
		auto getBuildPathStylePresetDescription = [](const std::string& preset) -> std::string {
			if (String::equals("target-triple", preset))
				return std::string("The target architecture's triple - ex: build/x64-linux-gnu_Debug");
			else if (String::equals("toolchain-name", preset))
				return std::string("The toolchain's name - ex: build/my-cool-toolchain_name_Debug");
			else if (String::equals("architecture", preset))
				return std::string("The architecture's identifier - ex: build/x86_64_Debug");
			else if (String::equals("configuration", preset))
				return std::string("Just the build configuration - ex: build/Debug");

			return std::string();
		};

		help += "\nBuild path styles:\n";
		StringList styles = CompilerTools::getToolchainBuildPathStyles();

		for (auto& strat : styles)
		{
			std::string line = strat;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getBuildPathStylePresetDescription(strat);

			help += fmt::format("{}\n", line);
		}
	}

	if (m_route == RouteType::Export)
	{
		auto getExportPresetDescription = [](const std::string& preset) -> std::string {
			if (String::equals("vscode", preset))
				return std::string("Visual Studio Code JSON format (launch.json, tasks.json, c_cpp_properties.json)");
#if defined(CHALET_WIN32)
			else if (String::equals("vssolution", preset))
				return std::string("Visual Studio Solution format (*.sln, *.vcxproj)");
			else if (String::equals("vsjson", preset))
				return std::string("Visual Studio JSON format (launch.vs.json, tasks.vs.json, CppProperties.json)");
#elif defined(CHALET_MACOS)
			else if (String::equals("xcode", preset))
				return fmt::format("Apple{} Xcode project format (*.xcodeproj)", Unicode::registered());
#endif
			else if (String::equals("clion", preset))
				return std::string("Jetbrains CLion (.idea)");
			else if (String::equals("fleet", preset))
				return std::string("Jetbrains Fleet (.fleet)");
			else if (String::equals("codeblocks", preset))
#if defined(CHALET_WIN32)
				return std::string("Code::Blocks IDE (MinGW-only)");
#else
				return std::string("Code::Blocks IDE (GCC-only)");
#endif

			return std::string();
		};

		help += "\nExport presets:\n";
		StringList exportPresets
		{
			"vscode",
#if defined(CHALET_WIN32)
				"vssolution",
				"vsjson",
#elif defined(CHALET_MACOS)
				"xcode",
#endif
				"clion",
				"fleet",
				"codeblocks",
		};

		for (auto& preset : exportPresets)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getExportPresetDescription(preset);

			help += fmt::format("{}\n", line);
		}
	}
	else if (m_route == RouteType::Convert)
	{
		auto getFormatDescription = [](const std::string& preset) -> std::string {
			if (String::equals("json", preset))
				return std::string("JSON: JavaScript Object Notation");
			else if (String::equals("yaml", preset))
				return std::string("YAML Ain't Markup Language");

			return std::string();
		};

		help += "\nBuild file formats:\n";

		StringList exportPresets = m_inputs.getConvertFormatPresets();

		for (auto& preset : exportPresets)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getFormatDescription(preset);

			help += fmt::format("{}\n", line);
		}
	}

	help += "\nApplication Paths:\n";
	StringList applicationPaths{
		"~/.chalet/",
		fmt::format("~/{}", m_inputs.globalSettingsFile()),
	};
	{
		u32 i = 0;
		for (auto& preset : applicationPaths)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			if (i == 0)
				line += "The global directory for settings across projects and future needs";
			else
				line += "The global settings file, where defaults and toolchains are set across projects";

			help += fmt::format("{}\n", line);
			i++;
		}
	}

	return help;
}

/*****************************************************************************/
/*****************************************************************************/
MappedArgument& ArgumentParser::addStringArgument(const ArgumentIdentifier inId, const char* inArg, std::string inDefaultValue)
{
	return m_argumentList.emplace_back(inId, Variant::Kind::String)
		.addArgument(inArg)
		.setValue(std::move(inDefaultValue));
}

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
		.addBooleanArgument(inArgument);
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
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(RouteType::Init)));

	subcommands.push_back("configure,c");
	descriptions.push_back(m_routeDescriptions.at(RouteType::Configure));

	subcommands.push_back(fmt::format("buildrun,r {} {}", Arg::BuildTarget, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(RouteType::BuildRun));

	subcommands.push_back(fmt::format("run {} {}", Arg::BuildTarget, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(RouteType::Run));

	subcommands.push_back("build,b");
	descriptions.push_back(m_routeDescriptions.at(RouteType::Build));

	subcommands.push_back("rebuild");
	descriptions.push_back(m_routeDescriptions.at(RouteType::Rebuild));

	subcommands.push_back("clean");
	descriptions.push_back(m_routeDescriptions.at(RouteType::Clean));

	subcommands.push_back("bundle");
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(RouteType::Bundle)));

	subcommands.push_back(fmt::format("get {}", Arg::SettingsKey));
	descriptions.push_back(m_routeDescriptions.at(RouteType::SettingsGet));

	subcommands.push_back(fmt::format("getkeys {}", Arg::SettingsKeyQuery));
	descriptions.push_back(m_routeDescriptions.at(RouteType::SettingsGetKeys));

	subcommands.push_back(fmt::format("set {} {}", Arg::SettingsKey, Arg::SettingsValue));
	descriptions.push_back(m_routeDescriptions.at(RouteType::SettingsSet));

	subcommands.push_back(fmt::format("unset {}", Arg::SettingsKey));
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(RouteType::SettingsUnset)));

	subcommands.push_back(fmt::format("convert {}", Arg::ConvertFormat));
	descriptions.push_back(m_routeDescriptions.at(RouteType::Convert));

	subcommands.push_back(fmt::format("export {}", Arg::ExportKind));
	descriptions.push_back(m_routeDescriptions.at(RouteType::Export));

	subcommands.push_back(fmt::format("validate {} {}", Arg::ValidateSchema, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(RouteType::Validate));

	subcommands.push_back(fmt::format("query {} {}", Arg::QueryType, Arg::RemainingArguments));
	descriptions.push_back(m_routeDescriptions.at(RouteType::Query));

	subcommands.push_back("termtest");
	descriptions.push_back(m_routeDescriptions.at(RouteType::TerminalTest));

	std::string help;
	if (subcommands.size() == descriptions.size())
	{
		for (size_t i = 0; i < subcommands.size(); ++i)
		{
			std::string line = subcommands.at(i);
			while (line.size() < kColumnSize)
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
	addTwoStringArguments(ArgumentIdentifier::TargetArchitecture, "-a", "--arch")
		.setHelp("The architecture to target for the build.");
}

/*****************************************************************************/
void ArgumentParser::addBuildStrategyArg()
{
	const auto& defaultValue = m_inputs.defaultBuildStrategy();
	addTwoStringArguments(ArgumentIdentifier::BuildStrategy, "-b", "--build-strategy")
		.setHelp(fmt::format("The build strategy to use for the selected toolchain. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addBuildPathStyleArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildPathStyle, "-p", "--build-path-style")
		.setHelp("The build path style, with the configuration appended by an underscore.");
}

/*****************************************************************************/
void ArgumentParser::addSaveSchemaArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::SaveSchema, "--save-schema")
		.setHelp("Save build & settings schemas to file.");
}

/*****************************************************************************/
void ArgumentParser::addSaveUserToolchainGloballyArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::SaveUserToolchainGlobally, "--save-user-toolchain-globally")
		.setHelp("Save the current or generated toolchain globally and make it the default.");
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
void ArgumentParser::addBuildTargetArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildTargetName, Positional::Argument2, Arg::BuildTarget)
		.setHelp("A build target to select. [default: \"all\"]");
}

/*****************************************************************************/
void ArgumentParser::addRunTargetArg()
{
	addTwoStringArguments(ArgumentIdentifier::BuildTargetName, Positional::Argument2, Arg::BuildTarget)
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
	addOptionalBoolArgument(ArgumentIdentifier::DumpAssembly, "--[no-]dump-assembly")
		.setHelp("Create an .asm dump of each object file during the build.");
}

/*****************************************************************************/
void ArgumentParser::addGenerateCompileCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::GenerateCompileCommands, "--[no-]generate-compile-commands")
		.setHelp("Generate a compile_commands.json file for Clang tooling use.");
}

/*****************************************************************************/
void ArgumentParser::addOnlyRequiredArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::OnlyRequired, "--[no-]only-required")
		.setHelp("Only build targets required by the target given at the command line.");
}

/*****************************************************************************/
void ArgumentParser::addShowCommandsArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::ShowCommands, "--[no-]show-commands")
		.setHelp("Show the commands run during the build.");
}

/*****************************************************************************/
void ArgumentParser::addBenchmarkArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::Benchmark, "--[no-]benchmark")
		.setHelp("Show all build times - total build time, build targets, other steps.");
}

/*****************************************************************************/
void ArgumentParser::addLaunchProfilerArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::LaunchProfiler, "--[no-]launch-profiler")
		.setHelp("If running profile targets, launch the preferred profiler afterwards.");
}

/*****************************************************************************/
void ArgumentParser::addKeepGoingArg()
{
	addOptionalBoolArgument(ArgumentIdentifier::KeepGoing, "--[no-]keep-going")
		.setHelp("If there's a build error, continue as much of the build as possible.");
}

/*****************************************************************************/
void ArgumentParser::addSigningIdentityArg()
{
	addStringArgument(ArgumentIdentifier::SigningIdentity, "--signing-identity")
		.setHelp("The code-signing identity to use when bundling the application distribution.");
}

/*****************************************************************************/
void ArgumentParser::addOsTargetNameArg()
{
#if defined(CHALET_MACOS)
	const auto& defaultValue = m_inputs.getDefaultOsTargetName();
	addStringArgument(ArgumentIdentifier::OsTargetName, "--os-target-name")
		.setHelp(fmt::format("The name of the operating system to target the build for. [default: \"{}\"]", defaultValue));
#else
	addStringArgument(ArgumentIdentifier::OsTargetName, "--os-target-name")
		.setHelp("The name of the operating system to target the build for.");
#endif
}

/*****************************************************************************/
void ArgumentParser::addOsTargetVersionArg()
{
#if defined(CHALET_MACOS)
	const auto& defaultValue = m_inputs.getDefaultOsTargetVersion();
	addStringArgument(ArgumentIdentifier::OsTargetVersion, "--os-target-version")
		.setHelp(fmt::format("The version of the operating system to target the build for. [default: \"{}\"]", defaultValue));
#else
	addStringArgument(ArgumentIdentifier::OsTargetVersion, "--os-target-version")
		.setHelp("The version of the operating system to target the build for.");
#endif
}

/*****************************************************************************/
void ArgumentParser::populateBuildRunArguments()
{
	populateCommonBuildArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentParser::populateRunArguments()
{
	populateCommonBuildArguments();

	addRunTargetArg();
	addRunArgumentsArg();
}

/*****************************************************************************/
void ArgumentParser::populateCommonBuildArguments()
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
	addBuildStrategyArg();
	addBuildPathStyleArg();
	addEnvFileArg();
	addMaxJobsArg();
	addOsTargetNameArg();
	addOsTargetVersionArg();
	addSigningIdentityArg();
	addShowCommandsArg();
	addDumpAssemblyArg();
	addBenchmarkArg();
	addLaunchProfilerArg();
	addKeepGoingArg();
	addGenerateCompileCommandsArg();
	addOnlyRequiredArg();
	addSaveUserToolchainGloballyArg();
#if defined(CHALET_DEBUG)
	addSaveSchemaArg();
#endif
	addQuietArgs();
}

/*****************************************************************************/
void ArgumentParser::populateBuildArguments()
{
	populateCommonBuildArguments();

	addBuildTargetArg();
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
	addInputFileArg();
	addSettingsFileArg();
	addRootDirArg();
	addExternalDirArg();
	addOutputDirArg();
	addDistributionDirArg();
	addToolchainArg();
	addArchArg();
	addBuildPathStyleArg();
	addEnvFileArg();
	addOsTargetNameArg();
	addOsTargetVersionArg();
	addSigningIdentityArg();

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
void ArgumentParser::populateConvertArguments()
{
	addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file")
		.setHelp("The path to the build file to convert to another format.");

	addTwoStringArguments(ArgumentIdentifier::ConvertFormat, Positional::Argument2, Arg::ConvertFormat)
		.setHelp("The format to convert the build file to.");
}

/*****************************************************************************/
void ArgumentParser::populateValidateArguments()
{
	addTwoStringArguments(ArgumentIdentifier::ValidateSchemaFile, Positional::Argument2, Arg::ValidateSchema)
		.setHelp("A JSON schema (Draft 7) to validate files against. File requires '$schema'.")
		.setRequired();

	addTwoStringArguments(ArgumentIdentifier::ValidateFilesRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments)
		.setHelp("File(s) to be validated using the selected schema.")
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
void ArgumentParser::populateTerminalTestArguments()
{
}

/*****************************************************************************/
#if defined(CHALET_DEBUG)
void ArgumentParser::populateDebugArguments()
{
	populateCommonBuildArguments();
}
#endif
}
