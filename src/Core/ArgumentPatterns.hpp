/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARG_PARSE_PARSER_HPP
#define CHALET_ARG_PARSE_PARSER_HPP

#include "Core/ArgumentIdentifier.hpp"
#include "Libraries/ArgParse.hpp"
#include "Router/Route.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
struct MappedArgument
{
	ArgumentIdentifier id;
	Variant value;

	MappedArgument(ArgumentIdentifier inId, Variant inValue);
};

class ArgumentPatterns
{
	using ParserAction = std::function<void(ArgumentPatterns&)>;
	using ParserList = std::unordered_map<Route, ParserAction>;
	using RouteMap = std::map<std::string, Route>;			   // note: ordered
	using ArgumentMap = std::map<std::string, MappedArgument>; // note: ordered

public:
	ArgumentPatterns();

	bool parse(const StringList& inArguments);
	const ArgumentMap& arguments() const noexcept;

	Route route() const noexcept;

	StringList getRouteList();

private:
	ushort parseOption(const std::string& inString);

	Route getRouteFromString(const std::string& inValue);

	void makeParser();
	bool doParse(const StringList& inArguments);
	bool showHelp();
	bool populateArgumentMap(const StringList& inArguments);
	// void populateArgumentMap(const StringList& inArguments);
	std::string getHelp();

	argparse::Argument& addStringArgument(const ArgumentIdentifier inId, const char* inArgument);
	argparse::Argument& addStringArgument(const ArgumentIdentifier inId, const char* inArgument, std::string inDefaultValue);
	argparse::Argument& addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue = std::string());
	argparse::Argument& addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong);
	argparse::Argument& addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue);
	argparse::Argument& addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument);
	argparse::Argument& addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue);
	argparse::Argument& addRemainingArguments(const ArgumentIdentifier inId, const char* inArgument);

	void populateMainArguments();
	void addInputFileArg();
	void addSettingsFileArg();
	void addFileArg();
	void addRootDirArg();
	void addExternalDirArg();
	void addOutputDirArg();
	void addBundleDirArg();
	void addProjectGenArg();
	void addToolchainArg();
	void addArchArg();
	void addMaxJobsArg();
	void addEnvFileArg();
	void addBuildConfigurationArg();
	void addRunTargetArg();
	void addRunArgumentsArg();
	void addSaveSchemaArg();
	void addQuietArgs();
	void addSettingsTypeArg();
	void addDumpAssemblyArg();
	void addGenerateCompileCommandsArg();
	void addShowCommandsArg();
	void addBenchmarkArg();

	void addOptionalArguments();
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
	void commandSettingsGetKeys();
	void commandSettingsSet();
	void commandSettingsUnset();
	void commandList();

#if defined(CHALET_DEBUG)
	void commandDebug();
#endif

	argparse::ArgumentParser m_parser;
	ParserList m_subCommands;

	StringList m_optionPairsCache;
	ArgumentMap m_argumentMap;
	RouteMap m_routeMap;

	std::string m_routeString;

	const std::string kArgRunTarget;
	const std::string kArgRemainingArguments;
	const std::string kArgInitName;
	const std::string kArgInitPath;
	const std::string kArgSettingsKey;
	const std::string kArgSettingsKeyQuery;
	const std::string kArgSettingsValue;

	Route m_route;
};
}

#endif // CHALET_ARG_PARSE_PARSER_HPP
