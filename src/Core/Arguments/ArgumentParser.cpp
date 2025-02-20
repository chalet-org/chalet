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
		{ RouteType::Check, &ArgumentParser::populateCommonBuildArguments },
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
		{ RouteType::Check, "Outputs the processed build file for the platform and selected toolchain." },
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
		{ "check", RouteType::Check },
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
		"--open",
	};
}

/*****************************************************************************/
bool ArgumentParser::resolveFromArguments(const i32 argc, const char* argv[])
{
	constexpr u32 maxPositionalArgs = 2;
	if (!parse(getArgumentList(argc, argv), maxPositionalArgs))
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

		auto& arg = m_argumentList.emplace_back(ArgumentIdentifier::RouteString, true);
		arg.addArgument(Positional::Argument1, routeString);
		arg.setHelp("This subcommand.");
		arg.setRequired();
	}
	else
	{
		addVersionArg();
	}
}

/*****************************************************************************/
void ArgumentParser::checkRemainingArguments()
{
	if (m_remainingArguments.empty())
		return;

	for (const auto& arg : m_argumentList)
	{
		if (String::equals(Positional::RemainingArguments, arg.key()))
			return;
	}

	if (containsOption(Positional::RemainingArguments))
	{
		auto remainingArgs = m_rawArguments.at(Positional::RemainingArguments);
		m_rawArguments.erase(Positional::RemainingArguments);
	}

	std::string blankArg;
	size_t i = 0;
	auto it = m_remainingArguments.begin();
	while (it != m_remainingArguments.end())
	{
		auto& arg = (*it);
		bool found = false;
		for (auto& mapped : m_argumentList)
		{
			if (String::equals(mapped.key(), arg) || String::equals(mapped.keyLong(), arg))
			{
				found = true;
				break;
			}
		}

		if (found)
		{
			auto next = it + 1;

			size_t j = i;

			{
				auto& nextArg = next != m_remainingArguments.end() ? (*next) : blankArg;
				parseArgumentValue(nextArg);
				parseArgument(i, arg, nextArg);
			}

			if (i > j && next != m_remainingArguments.end())
			{
				next = m_remainingArguments.erase(next);
			}
			it = m_remainingArguments.erase(it);
		}
		else
		{
			it++;
		}

		i++;
	}
}

/*****************************************************************************/
bool ArgumentParser::doParse()
{
	checkRemainingArguments();

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
		if (containsOption(Positional::Argument1) && m_routeString.empty())
		{
			Diagnostic::error("Invalid subcommand: '{}'. See 'chalet --help'.", m_rawArguments.at(Positional::Argument1));
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
	bool containsRemaining = !m_remainingArguments.empty();
	bool allowsRemaining = false;

	i32 maxPositionalArgs = 0;

	StringList allArguments;

	for (auto& mapped : m_argumentList)
	{
		bool isRemaining = String::equals(Positional::RemainingArguments, mapped.key());
		bool isPositional = String::startsWith('@', mapped.key());

		if (isPositional)
			maxPositionalArgs++;

		if (mapped.id() == ArgumentIdentifier::RouteString)
			continue;

		allowsRemaining |= isRemaining;

		if (!isRemaining && !mapped.key().empty())
			allArguments.emplace_back(mapped.key());

		if (!isRemaining && !isPositional && !mapped.keyLong().empty())
			allArguments.emplace_back(mapped.keyLong());

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
					mapped.setValue(std::optional<i32>(atoi(value.c_str())));

				break;
			}

			case Variant::Kind::String:
				mapped.setValue(value);
				break;

			case Variant::Kind::StringList: {
				if (isRemaining)
					mapped.setValue(m_remainingArguments);

				break;
			}
			case Variant::Kind::Empty:
			default:
				break;
		}
	}

	StringList invalid;
	bool remainingNotAllowed = containsRemaining && !allowsRemaining;
	if (remainingNotAllowed)
	{
		invalid = m_remainingArguments;
	}

	i32 positionalArgs = 0;
	for (auto& [key, arg] : m_rawArguments)
	{
		if (String::equals(Positional::ProgramArgument, key))
			continue;

		if (String::startsWith('@', key))
		{
			positionalArgs++;

			if (String::equals(m_routeString, arg))
				continue;

			if (positionalArgs > maxPositionalArgs)
				invalid.emplace_back(arg);

			continue;
		}

		if (String::equals(Positional::RemainingArguments, key))
			continue;

		if (!List::contains(allArguments, key))
		{
			invalid.emplace_back(key);
		}
	}

	if (!invalid.empty())
	{
		auto seeHelp = getSeeHelpMessage();
		for (auto& arg : invalid)
		{
			Diagnostic::error("Unknown argument: '{}'. {}", arg, seeHelp);
		}
		return false;
	}

	// fallback in case something is broken
	if (positionalArgs > maxPositionalArgs)
	{
		auto seeHelp = getSeeHelpMessage();
		Diagnostic::error("Maximum number of positional arguments exceeded. {}", seeHelp);
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
				return "The LLVM Compiler Infrastructure Project";
			else if (String::equals("emscripten", preset))
				return "Emscripten compiler toolchain for WebAssembly";
#if defined(CHALET_WIN32)
			else if (String::equals("gcc", preset))
				return "MinGW: Minimalist GNU Compiler Collection for Windows";
#else
			else if (String::equals("gcc", preset))
				return "GNU Compiler Collection";
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
				return "Build with Ninja";
			else if (String::equals("makefile", preset))
#if defined(CHALET_WIN32)
				return "Build with GNU Make (MinGW), NMake or Qt Jom (MSVC)";
#else
				return "Build with GNU Make";
#endif
			else if (String::equals("native", preset))
				return "Build natively with Chalet";
#if defined(CHALET_WIN32)
			else if (String::equals("msbuild", preset))
				return "Build using a Visual Studio solution and MSBuild - requires vs-* toolchain preset";
#elif defined(CHALET_MACOS)
			else if (String::equals("xcodebuild", preset))
				return "Build using an Xcode project and xcodebuild - requires apple-llvm toolchain preset";
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
				return "The target architecture's triple - ex: build/x64-linux-gnu_Debug";
			else if (String::equals("toolchain-name", preset))
				return "The toolchain's name - ex: build/my-cool-toolchain_name_Debug";
			else if (String::equals("architecture", preset))
				return "The architecture's identifier - ex: build/x86_64_Debug";
			else if (String::equals("configuration", preset))
				return "Just the build configuration - ex: build/Debug";

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
				return "Visual Studio Code (.vscode)";
#if defined(CHALET_WIN32)
			else if (String::equals("vssolution", preset))
				return "Visual Studio Solution format (*.sln, *.vcxproj)";
			else if (String::equals("vsjson", preset))
				return "Visual Studio JSON format (launch.vs.json, tasks.vs.json, CppProperties.json)";
#elif defined(CHALET_MACOS)
			else if (String::equals("xcode", preset))
				return fmt::format("Apple{} Xcode project format (*.xcodeproj)", Unicode::registered());
			else if (String::equals("codeedit", preset))
				return "CodeEdit for macOS (.codeedit)";
#endif
			else if (String::equals("clion", preset))
				return "Jetbrains CLion (.idea)";
			else if (String::equals("fleet", preset))
				return "Jetbrains Fleet (.fleet)";
			else if (String::equals("codeblocks", preset))
#if defined(CHALET_WIN32)
				return "Code::Blocks IDE (MinGW-only)";
#else
				return "Code::Blocks IDE (GCC-only)";
#endif

			return std::string();
		};

		help += "\nExport project types:\n";
		StringList exportPresets{
			"vscode",
#if defined(CHALET_WIN32)
			"vssolution",
			"vsjson",
#elif defined(CHALET_MACOS)
			"xcode",
			"codeedit",
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
	else if (m_route == RouteType::Query)
	{
		auto getQueryDescription = [](const std::string& preset) -> std::string {
			if (String::equals("all-toolchains", preset))
				return "A list of all user toolchain and built-in preset names.";
			else if (String::equals("architecture", preset))
				return "The current toolchain architecture.";
			else if (String::equals("architectures", preset))
				return "A list of all available toolchain architectures and aliases.";
			else if (String::equals("options", preset))
				return "A list of all the cli options (regardless of subcommand).";
			else if (String::equals("commands", preset))
				return "A list of all of the chalet subcommands.";
			else if (String::equals("configuration", preset))
				return "The current build configuration.";
			else if (String::equals("configurations", preset))
				return "A list of all available build configurations for the project.";
			else if (String::equals("list-names", preset))
				return "A list of all query types (this list).";
			else if (String::equals("export-kinds", preset))
				return "A list of the available export kinds.";
			else if (String::equals("convert-formats", preset))
				return "A list of the available convert formats.";
			else if (String::equals("run-target", preset))
				return "The current run target set.";
			else if (String::equals("all-build-targets", preset))
				return "A list of the available build targets in the project.";
			else if (String::equals("all-run-targets", preset))
				return "A list of the available run targets in the project.";
			else if (String::equals("theme-names", preset))
				return "A list of the available theme names.";
			else if (String::equals("toolchain", preset))
				return "The current toolchain name.";
			else if (String::equals("toolchain-presets", preset))
				return "A list of the built-in toolchain presets for the platform.";
			else if (String::equals("user-toolchains", preset))
				return "A list of the user-created toolchains (if any).";
			else if (String::equals("build-strategy", preset))
				return "The current build strategy for the selected toolchain.";
			else if (String::equals("build-strategies", preset))
				return "A list of the available build strategies for the platform.";
			else if (String::equals("build-path-style", preset))
				return "The current build path style for the selected toolchain.";
			else if (String::equals("build-path-styles", preset))
				return "A list of the available build path styles.";
			else if (String::equals("state-chalet-json", preset))
				return "A json structure describing the current project state.";
			else if (String::equals("state-settings-json", preset))
				return "A json structure describing the current configured state.";
			else if (String::equals("schema-chalet-json", preset))
				return "The build file schema in JSON format.";
			else if (String::equals("schema-settings-json", preset))
				return "The settings file schema in JSON format.";
			else if (String::equals("version", preset))
				return "The Chalet version.";

			return std::string();
		};

		help += "\nQuery types:\n";

		auto queryOptions = m_inputs.getCliQueryOptions();

		for (auto& preset : queryOptions)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getQueryDescription(preset);

			help += fmt::format("{}\n", line);
		}
	}
	else if (m_route == RouteType::Convert)
	{
		auto getFormatDescription = [](const std::string& preset) -> std::string {
			if (String::equals("json", preset))
				return "JSON: JavaScript Object Notation";
			else if (String::equals("yaml", preset))
				return "YAML Ain't Markup Language";

			return std::string();
		};

		help += "\nBuild file formats:\n";

		StringList convertPresets = m_inputs.getConvertFormatPresets();
		for (auto& preset : convertPresets)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getFormatDescription(preset);

			help += fmt::format("{}\n", line);
		}
	}
	else if (m_route == RouteType::Init)
	{
		auto getFormatDescription = [](const std::string& preset) -> std::string {
			if (String::equals("chalet", preset))
				return "A chalet.json with a single executable target";
			else if (String::equals("cmake", preset))
				return "A chalet.json with a single CMake target and CMakeLists.txt";

			return std::string();
		};

		help += "\nProject templates:\n";

		StringList initPresets = m_inputs.getProjectInitializationPresets();
		for (auto& preset : initPresets)
		{
			std::string line = preset;
			while (line.size() < kColumnSize)
				line += ' ';
			line += '\t';
			line += getFormatDescription(preset);

			help += fmt::format("{}\n", line);
		}
	}

	help += "\nApplication paths:\n";
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
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::String);
	arg.addArgument(inArg);
	arg.setValue(std::move(inDefaultValue));
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::String);
	arg.addArgument(inShort, inLong);
	arg.setValue(std::move(inDefaultValue));
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoStringListArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, StringList inDefaultValue)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::StringList);
	arg.addArgument(inShort, inLong);
	arg.setValue(std::move(inDefaultValue));
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::OptionalInteger);
	arg.addArgument(inShort, inLong);
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::Boolean);
	arg.addArgument(inArgument);
	arg.setValue(inDefaultValue);
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::OptionalBoolean);
	arg.addBooleanArgument(inArgument);
	return arg;
}

/*****************************************************************************/
MappedArgument& ArgumentParser::addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue)
{
	auto& arg = m_argumentList.emplace_back(inId, Variant::Kind::Boolean);
	arg.addArgument(inShort, inLong);
	arg.setValue(inDefaultValue);
	return arg;
}

/*****************************************************************************/
void ArgumentParser::populateMainArguments()
{
	StringList subcommands;
	StringList descriptions;

	subcommands.push_back(fmt::format("init [{}]", Arg::InitPath));
	descriptions.push_back(fmt::format("{}\n", m_routeDescriptions.at(RouteType::Init)));

	subcommands.push_back("check");
	descriptions.push_back(m_routeDescriptions.at(RouteType::Check));

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

	auto& arg = addBoolArgument(ArgumentIdentifier::SubCommand, "<subcommand>", true);
	arg.setHelp(std::move(help));
}

/*****************************************************************************/
void ArgumentParser::addHelpArg()
{
	auto& arg = addTwoBoolArguments(ArgumentIdentifier::Help, "-h", "--help", false);
	arg.setHelp("Shows help message (if applicable, for the subcommand) and exits.");
}

/*****************************************************************************/
void ArgumentParser::addVersionArg()
{
	auto& arg = addTwoBoolArguments(ArgumentIdentifier::Version, "-v", "--version", false);
	arg.setHelp("Prints version information and exits.");
}

/*****************************************************************************/
void ArgumentParser::addInputFileArg()
{
	const auto& defaultValue = m_inputs.defaultInputFile();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file");
	arg.setHelp(fmt::format("An input build file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addSettingsFileArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::SettingsFile, "-s", "--settings-file");
	arg.setHelp(fmt::format("The path to a settings file to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addFileArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::File, "-f", "--file");
	arg.setHelp("The path to a JSON file to examine, if not the local/global settings.");
}

/*****************************************************************************/
void ArgumentParser::addRootDirArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::RootDirectory, "-r", "--root-dir");
	arg.setHelp("The root directory to run the build from. [default: \".\"]");
}

/*****************************************************************************/
void ArgumentParser::addOutputDirArg()
{
	const auto& defaultValue = m_inputs.defaultOutputDirectory();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::OutputDirectory, "-o", "--output-dir");
	arg.setHelp(fmt::format("The output directory of the build. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addExternalDirArg()
{
	const auto& defaultValue = m_inputs.defaultExternalDirectory();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::ExternalDirectory, "-x", "--external-dir");
	arg.setHelp(fmt::format("The directory to install external dependencies into. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addDistributionDirArg()
{
	const auto& defaultValue = m_inputs.defaultDistributionDirectory();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::DistributionDirectory, "-d", "--distribution-dir");
	arg.setHelp(fmt::format("The root directory for all distribution bundles. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addToolchainArg()
{
	const auto& defaultValue = m_inputs.defaultToolchainPreset();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::Toolchain, "-t", "--toolchain");
	arg.setHelp(fmt::format("A toolchain or toolchain preset to use. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addMaxJobsArg()
{
	auto jobs = std::thread::hardware_concurrency();
	auto& arg = addTwoIntArguments(ArgumentIdentifier::MaxJobs, "-j", "--max-jobs");
	arg.setHelp(fmt::format("The number of jobs to run during compilation. [default: {}]", jobs));
}

/*****************************************************************************/
void ArgumentParser::addEnvFileArg()
{
	const auto& defaultValue = m_inputs.defaultEnvFile();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::EnvFile, "-e", "--env-file");
	arg.setHelp(fmt::format("A file to load environment variables from. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addArchArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::TargetArchitecture, "-a", "--arch");
	arg.setHelp("The architecture to target for the build.");
}

/*****************************************************************************/
void ArgumentParser::addBuildStrategyArg()
{
	const auto& defaultValue = m_inputs.defaultBuildStrategy();
	auto& arg = addTwoStringArguments(ArgumentIdentifier::BuildStrategy, "-b", "--build-strategy");
	arg.setHelp(fmt::format("The build strategy to use for the selected toolchain. [default: \"{}\"]", defaultValue));
}

/*****************************************************************************/
void ArgumentParser::addBuildPathStyleArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::BuildPathStyle, "-p", "--build-path-style");
	arg.setHelp("The build path style, with the configuration appended by an underscore.");
}

/*****************************************************************************/
void ArgumentParser::addSaveSchemaArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::SaveSchema, "--save-schema");
	arg.setHelp("Save build & settings schemas to file.");
}

/*****************************************************************************/
void ArgumentParser::addSaveUserToolchainGloballyArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::SaveUserToolchainGlobally, "--save-user-toolchain-globally");
	arg.setHelp("Save the current or generated toolchain globally and make it the default.");
}

/*****************************************************************************/
void ArgumentParser::addQuietArgs()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::Quieter, "--quieter");
	arg.setHelp("Show only the build output.");
}

/*****************************************************************************/
void ArgumentParser::addBuildConfigurationArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::BuildConfiguration, "-c", "--configuration");
	arg.setHelp("The build configuration to use. [default: \"Release\"]");
}

/*****************************************************************************/
void ArgumentParser::addExportBuildConfigurationsArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::ExportBuildConfigurations, "-c", "--configurations");
	arg.setHelp("The build configuration(s) to export, separated by comma.");
}

/*****************************************************************************/
void ArgumentParser::addExportArchitecturesArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::ExportArchitectures, "-a", "--architectures");
	arg.setHelp("The architecture(s) to export, separated by comma.");
}

/*****************************************************************************/
void ArgumentParser::addBuildTargetArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::BuildTargetName, Positional::Argument2, Arg::BuildTarget);
	arg.setHelp("A build target to select. [default: \"all\"]");
}

/*****************************************************************************/
void ArgumentParser::addRunTargetArg()
{
	auto& arg = addTwoStringArguments(ArgumentIdentifier::BuildTargetName, Positional::Argument2, Arg::BuildTarget);
	arg.setHelp("An executable or script target to run.");
}

/*****************************************************************************/
void ArgumentParser::addRunArgumentsArg()
{
	auto& arg = addTwoStringListArguments(ArgumentIdentifier::RunTargetArguments, Positional::RemainingArguments, Arg::RemainingArguments);
	arg.setHelp("The arguments to pass to the run target.");
}

/*****************************************************************************/
void ArgumentParser::addSettingsTypeArg()
{
	const auto& defaultValue = m_inputs.defaultSettingsFile();
	auto& arg1 = addTwoBoolArguments(ArgumentIdentifier::LocalSettings, "-l", "--local", false);
	arg1.setHelp(fmt::format("Use the local settings. [{}]", defaultValue));

	const auto& globalSettings = m_inputs.globalSettingsFile();
	auto& arg2 = addTwoBoolArguments(ArgumentIdentifier::GlobalSettings, "-g", "--global", false);
	arg2.setHelp(fmt::format("Use the global settings. [~/{}]", globalSettings));
}

/*****************************************************************************/
void ArgumentParser::addDumpAssemblyArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::DumpAssembly, "--[no-]dump-assembly");
	arg.setHelp("Create an .asm dump of each object file during the build.");
}

/*****************************************************************************/
void ArgumentParser::addGenerateCompileCommandsArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::GenerateCompileCommands, "--[no-]generate-compile-commands");
	arg.setHelp("Generate a compile_commands.json file for Clang tooling use.");
}

/*****************************************************************************/
void ArgumentParser::addOnlyRequiredArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::OnlyRequired, "--[no-]only-required");
	arg.setHelp("Only build targets required by the target given at the command line.");
}

/*****************************************************************************/
void ArgumentParser::addShowCommandsArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::ShowCommands, "--[no-]show-commands");
	arg.setHelp("Show the commands run during the build.");
}

/*****************************************************************************/
void ArgumentParser::addBenchmarkArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::Benchmark, "--[no-]benchmark");
	arg.setHelp("Show all build times - total build time, build targets, other steps.");
}

/*****************************************************************************/
void ArgumentParser::addLaunchProfilerArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::LaunchProfiler, "--[no-]launch-profiler");
	arg.setHelp("If running profile targets, launch the preferred profiler afterwards.");
}

/*****************************************************************************/
void ArgumentParser::addKeepGoingArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::KeepGoing, "--[no-]keep-going");
	arg.setHelp("If there's a build error, continue as much of the build as possible.");
}

/*****************************************************************************/
void ArgumentParser::addCompilerCacheArg()
{
	auto& arg = addOptionalBoolArgument(ArgumentIdentifier::CompilerCache, "--[no-]compiler-cache");
	arg.setHelp("Use a compiler cache (ie. ccache) if available.");
}

/*****************************************************************************/
void ArgumentParser::addSigningIdentityArg()
{
	auto& arg = addStringArgument(ArgumentIdentifier::SigningIdentity, "--signing-identity");
	arg.setHelp("The code-signing identity to use when bundling the application distribution.");
}

/*****************************************************************************/
void ArgumentParser::addOsTargetNameArg()
{
#if defined(CHALET_MACOS)
	const auto defaultValue = m_inputs.getDefaultOsTargetName();
	auto& arg = addStringArgument(ArgumentIdentifier::OsTargetName, "--os-target-name");
	arg.setHelp(fmt::format("The name of the operating system to target the build for. [default: \"{}\"]", defaultValue));
#else
	auto& arg = addStringArgument(ArgumentIdentifier::OsTargetName, "--os-target-name");
	arg.setHelp("The name of the operating system to target the build for.");
#endif
}

/*****************************************************************************/
void ArgumentParser::addOsTargetVersionArg()
{
#if defined(CHALET_MACOS)
	const auto defaultValue = m_inputs.getDefaultOsTargetVersion();
	auto& arg = addStringArgument(ArgumentIdentifier::OsTargetVersion, "--os-target-version");
	arg.setHelp(fmt::format("The version of the operating system to target the build for. [default: \"{}\"]", defaultValue));
#else
	auto& arg = addStringArgument(ArgumentIdentifier::OsTargetVersion, "--os-target-version");
	arg.setHelp("The version of the operating system to target the build for.");
#endif
}

/*****************************************************************************/
void ArgumentParser::addExportOpenArg()
{
	auto& arg = addBoolArgument(ArgumentIdentifier::ExportOpen, "--open", false);
	arg.setHelp("Open the project in its associated editor after exporting.");
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
	addCompilerCacheArg();
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
	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::InitTemplate, "-t", "--template");
	arg1.setHelp(fmt::format("The project template to use during initialization. [default: \"{}\"]", templates.front()));

	auto& arg2 = addTwoStringArguments(ArgumentIdentifier::InitPath, Positional::Argument2, Arg::InitPath, ".");
	arg2.setHelp("The path of the project to initialize. [default: \".\"]");
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
	addExportBuildConfigurationsArg();
	addToolchainArg();
	addExportArchitecturesArg();
	addBuildPathStyleArg();
	addEnvFileArg();
	addOsTargetNameArg();
	addOsTargetVersionArg();
	addSigningIdentityArg();
	addShowCommandsArg();
	addBenchmarkArg();
	addExportOpenArg();

	auto& arg = addTwoStringArguments(ArgumentIdentifier::ExportKind, Positional::Argument2, Arg::ExportKind);
	arg.setHelp("The project type to export to. (see below)");
	arg.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateSettingsGetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	auto& arg = addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey);
	arg.setHelp("The config key to get.");
}

/*****************************************************************************/
void ArgumentParser::populateSettingsGetKeysArguments()
{
	addFileArg();
	addSettingsTypeArg();

	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKeyQuery);
	arg1.setHelp("The config key to query for.");

	auto& arg2 = addTwoStringListArguments(ArgumentIdentifier::SettingsKeysRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments);
	arg2.setHelp("Additional query arguments, if applicable.");
}

/*****************************************************************************/
void ArgumentParser::populateSettingsSetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey);
	arg1.setHelp("The config key to change.");
	arg1.setRequired();

	auto& arg2 = addTwoStringListArguments(ArgumentIdentifier::SettingsValueRemainingArgs, Positional::RemainingArguments, Arg::SettingsValue);
	arg2.setHelp("The config value to change to.");
	arg2.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateSettingsUnsetArguments()
{
	addFileArg();
	addSettingsTypeArg();

	auto& arg = addTwoStringArguments(ArgumentIdentifier::SettingsKey, Positional::Argument2, Arg::SettingsKey);
	arg.setHelp("The config key to remove.");
	arg.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateConvertArguments()
{
	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::InputFile, "-i", "--input-file");
	arg1.setHelp("The path to the build file to convert to another format.");

	auto& arg2 = addTwoStringArguments(ArgumentIdentifier::ConvertFormat, Positional::Argument2, Arg::ConvertFormat);
	arg2.setHelp("The format to convert the build file to.");
}

/*****************************************************************************/
void ArgumentParser::populateValidateArguments()
{
	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::ValidateSchemaFile, Positional::Argument2, Arg::ValidateSchema);
	arg1.setHelp("A JSON schema (Draft 7) to validate files against. File requires '$schema'.");
	arg1.setRequired();

	auto& arg2 = addTwoStringListArguments(ArgumentIdentifier::ValidateFilesRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments);
	arg2.setHelp("File(s) to be validated using the selected schema.");
	arg2.setRequired();
}

/*****************************************************************************/
void ArgumentParser::populateQueryArguments()
{
	auto& arg1 = addTwoStringArguments(ArgumentIdentifier::QueryType, Positional::Argument2, Arg::QueryType);
	arg1.setHelp("The data type to query for.");
	arg1.setRequired();

	auto& arg2 = addTwoStringListArguments(ArgumentIdentifier::QueryDataRemainingArgs, Positional::RemainingArguments, Arg::RemainingArguments);
	arg2.setHelp("Data to provide to the query. (architecture: <toolchain-name>)");
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
