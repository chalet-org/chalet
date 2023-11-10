/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Arguments/ArgumentIdentifier.hpp"
#include "Arguments/BaseArgumentParser.hpp"
#include "Arguments/MappedArgument.hpp"
#include "Router/CommandRoute.hpp"
#include "Utility/Variant.hpp"

namespace chalet
{
struct CommandLineInputs;

class ArgumentParser final : public BaseArgumentParser
{
	using ParserAction = std::function<void(ArgumentParser&)>;
	using ParserList = std::unordered_map<RouteType, ParserAction>;
	using RouteDescriptionList = std::unordered_map<RouteType, std::string>;
	using RouteMap = OrderedDictionary<RouteType>;
	using ArgumentList = std::vector<MappedArgument>;

public:
	ArgumentParser(const CommandLineInputs& inInputs);

	bool resolveFromArguments(const i32 argc, const char* argv[]);
	const ArgumentList& arguments() const noexcept;

	CommandRoute getRoute() const noexcept;

	StringList getRouteList() const;

	std::string getProgramPath() const;

	StringList getAllCliOptions();

private:
	virtual StringList getTruthyArguments() const final;

	RouteType getRouteFromString(const std::string& inValue) const;

	void makeParser();
	bool doParse();
	bool showHelp();
	bool showVersion();
	bool isSubcommand() const;
	bool assignArgumentListFromArgumentsAndValidate();
	std::string getHelp();
	std::string getSeeHelpMessage();

	MappedArgument& addStringArgument(const ArgumentIdentifier inId, const char* inArg, std::string inDefaultValue = std::string());
	MappedArgument& addTwoStringArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, std::string inDefaultValue = std::string());
	MappedArgument& addTwoIntArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong);
	MappedArgument& addBoolArgument(const ArgumentIdentifier inId, const char* inArgument, const bool inDefaultValue);
	MappedArgument& addOptionalBoolArgument(const ArgumentIdentifier inId, const char* inArgument);
	MappedArgument& addTwoBoolArguments(const ArgumentIdentifier inId, const char* inShort, const char* inLong, const bool inDefaultValue);

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
	void addToolchainArg();
	void addArchArg();
	void addBuildStrategyArg();
	void addBuildPathStyleArg();
	void addMaxJobsArg();
	void addEnvFileArg();
	void addBuildConfigurationArg();
	void addBuildTargetArg();
	void addRunTargetArg();
	void addRunArgumentsArg();
	void addSaveSchemaArg();
	void addSaveUserToolchainGloballyArg();
	void addQuietArgs();
	void addSettingsTypeArg();
	void addDumpAssemblyArg();
	void addGenerateCompileCommandsArg();
	void addOnlyRequiredArg();
	void addShowCommandsArg();
	void addBenchmarkArg();
	void addLaunchProfilerArg();
	void addKeepGoingArg();
	void addSigningIdentityArg();
	void addOsTargetNameArg();
	void addOsTargetVersionArg();

	void populateBuildRunArguments();
	void populateRunArguments();
	void populateCommonBuildArguments();
	void populateBuildArguments();

	void populateInitArguments();
	void populateExportArguments();
	void populateSettingsGetArguments();
	void populateSettingsGetKeysArguments();
	void populateSettingsSetArguments();
	void populateSettingsUnsetArguments();
	void populateValidateArguments();
	void populateQueryArguments();
	void populateTerminalTestArguments();

#if defined(CHALET_DEBUG)
	void populateDebugArguments();
#endif

	const CommandLineInputs& m_inputs;

	ParserList m_subCommands;
	RouteDescriptionList m_routeDescriptions;

	ArgumentList m_argumentList;
	RouteMap m_routeMap;

	std::string m_routeString;

	RouteType m_route;

	bool m_hasRemaining = false;
};
}
