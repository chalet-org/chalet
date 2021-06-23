/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARG_PARSE_PARSER_HPP
#define CHALET_ARG_PARSE_PARSER_HPP

#include "Libraries/ArgParse.hpp"
#include "Router/Route.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
class ArgumentPatterns
{
	using ParserAction = std::function<void(ArgumentPatterns&)>;
	using ParserList = std::unordered_map<Route, ParserAction>;
	using ArgumentMap = std::vector<std::pair<std::string, Variant>>;

public:
	ArgumentPatterns();

	const std::string& argConfiguration() const noexcept;
	const std::string& argRunProject() const noexcept;
	const std::string& argRunArguments() const noexcept;
	const std::string& argInitPath() const noexcept;
	const std::string& argSettingsKey() const noexcept;
	const std::string& argSettingsValue() const noexcept;

	bool parse(const StringList& inArguments);
	const ArgumentMap& arguments() const noexcept;

	Route route() const noexcept;

private:
	ushort parseOption(const std::string& inString);
	std::string getHelpCommand();

	Route getRouteFromString(const std::string& inValue);

	void makeParser();
	bool doParse(const StringList& inArguments);
	bool populateArgumentMap(const StringList& inArguments);
	// void populateArgumentMap(const StringList& inArguments);
	std::string getHelp();

	void populateMainArguments();
	void addInputFileArg();
	void addOutPathArg();
	void addProjectGeneratorArg();
	void addToolchainArg();
	void addEnvFileArg();
	void addArchArg();
	void addBuildConfigurationArg(const bool inOptional = false);
	void addRunProjectArg();
	void addRunArgumentsArg();
	void addSaveSchemaArg();
	void addQuietArgs();
	void addSettingsTypeArg();

	void commandBuildRun();
	void commandRun();
	void commandBuild();
	void commandRebuild();
	void commandClean();
	void commandBundle();
	// void commandInstall();
	void commandConfigure();
	void commandInit();
	void commandSettingsGet();
	void commandSettingsSet();
	void commandSettingsUnset();

#if defined(CHALET_DEBUG)
	void commandDebug();
#endif

	argparse::ArgumentParser m_parser;
	ParserList m_subCommands;

	ArgumentMap m_argumentMap;

	Route m_route;

	std::string m_routeString;

	const std::string kCommand = "<command>";

	const std::string kArgConfiguration = "<configuration>";
	const std::string kArgRunProject = "[<runProject>]";
	const std::string kArgRunArguments = "[ARG...]";
	const std::string kArgInitName = "<name>";
	const std::string kArgInitPath = "<path>";
	const std::string kArgSettingsKey = "<key>";
	const std::string kArgSettingsValue = "<value>";

	const std::string kHelpBuildConfiguration = "The build configuration";
	const std::string kHelpRunProject = "A project to run";
	const std::string kHelpRunArguments = "The arguments to pass to the run project";
	const std::string kHelpInitName = "The name of the project to initialize";
	const std::string kHelpInitPath = "The path of the project to initialize";
};
}

#endif // CHALET_ARG_PARSE_PARSER_HPP
