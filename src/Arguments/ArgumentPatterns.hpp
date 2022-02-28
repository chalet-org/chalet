/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ARG_PARSE_PARSER_HPP
#define CHALET_ARG_PARSE_PARSER_HPP

#include "Arguments/ArgumentIdentifier.hpp"
#include "Arguments/MappedArgument.hpp"
#include "Router/Route.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
struct CommandLineInputs;

class ArgumentPatterns
{
	using ParserAction = std::function<void(ArgumentPatterns&)>;
	using ParserList = std::unordered_map<Route, ParserAction>;
	using RouteMap = OrderedDictionary<Route>;
	using ArgumentList = std::map<ArgumentIdentifier, MappedArgument>;

public:
	ArgumentPatterns(const CommandLineInputs& inInputs);

	bool resolveFromArguments(const StringList& inArguments);
	const ArgumentList& arguments() const noexcept;

	Route route() const noexcept;

	StringList getRouteList();

private:
	ushort parseOption(const std::string& inString);

	Route getRouteFromString(const std::string& inValue);

	void makeParser();
	bool doParse(const StringList& inArguments);
	bool showHelp();
	bool showVersion();
	bool populateArgumentMap(const StringList& inArguments);
	std::string getHelp();

	MappedArgument& addStringArgument(const ArgumentIdentifier inId, const char* inArgument);
	MappedArgument& addStringArgument(const ArgumentIdentifier inId, const char* inArgument, std::string inDefaultValue);
	MappedArgument& addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue = std::string());
	MappedArgument& addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong);
	MappedArgument& addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue);
	MappedArgument& addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument);
	MappedArgument& addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue);
	MappedArgument& addRemainingArguments(const ArgumentIdentifier inId, const char* inArgument);

	void populateMainArguments();

	void addHelpArg();
	void addVersionArg();

	void addInputFileArg();
	void addSettingsFileArg();
	void addFileArg();
	void addRootDirArg();
	void addExternalDirArg();
	void addOutputDirArg();
	void addDistributionDirArg();
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
	void addLaunchProfilerArg();
	void addKeepGoingArg();
	void addOptionalArguments();

	void populateBuildRunArguments();
	void populateRunArguments();
	void populateBuildArguments();
	void populateRebuildArguments();
	void populateCleanArguments();
	void populateBundleArguments();
	void populateConfigureArguments();
	void populateInitArguments();
	void populateSettingsGetArguments();
	void populateSettingsGetKeysArguments();
	void populateSettingsSetArguments();
	void populateSettingsUnsetArguments();
	void populateQueryArguments();
	void populateColorTestArguments();

#if defined(CHALET_DEBUG)
	void populateDebugArguments();
#endif

	const CommandLineInputs& m_inputs;

	ParserList m_subCommands;

	StringList m_optionPairsCache;
	ArgumentList m_argumentList;
	RouteMap m_routeMap;

	std::string m_routeString;
	std::ptrdiff_t m_ignoreIndex = 0;

	Route m_route;
};
}

#endif // CHALET_ARG_PARSE_PARSER_HPP
