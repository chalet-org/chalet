/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_PROJECT_INITIALIZER_HPP
#define CHALET_PROJECT_INITIALIZER_HPP

#include "Compile/CodeLanguage.hpp"
#include "Compile/CxxSpecialization.hpp"

namespace chalet
{
struct BuildJsonProps;
struct CommandLineInputs;

struct ProjectInitializer
{
	explicit ProjectInitializer(const CommandLineInputs& inInputs);

	bool run();

private:
	bool initializeNormalWorkspace(BuildJsonProps& outProps);
	bool initializeCMakeWorkspace(BuildJsonProps& outProps);

	bool doRun(const BuildJsonProps& inProps);

	bool makeBuildJson(const BuildJsonProps& inProps);
	bool makeMainCpp(const BuildJsonProps& inProps);
	bool makePch(const BuildJsonProps& inProps);
	bool makeCMakeLists(const BuildJsonProps& inProps);
	bool makeGitIgnore();
	bool makeReadme();
	bool makeDotEnv();

	std::string getBannerV1() const;
	std::string getBannerV2() const;

	// questionaire
	std::string getWorkspaceName() const;
	std::string getWorkspaceVersion() const;
	std::string getProjectName(const std::string& inWorkspaceName) const;
	// lang bits
	std::string getRootSourceDirectory() const;
	std::string getMainSourceFile(const CodeLanguage inLang) const;
	std::string getCxxPrecompiledHeaderFile(const CodeLanguage inLang, const CxxSpecialization inCxxSpecialization) const;
	std::pair<CodeLanguage, CxxSpecialization> getCodeLanguage();
	std::string getLanguageStandard(const CodeLanguage inLang) const;

	bool getUseLocation() const;
	bool getIncludeDefaultBuildConfigurations() const;
	bool getMakeEnvFile() const;
	bool getMakeGitRepository() const;

	void printFileNameAndContents(const bool inCondition, const std::string& inFileName, const std::function<std::string()>& inGetContents) const;
	void printUserInputSplit() const;

	const CommandLineInputs& m_inputs;

	StringList m_sourceExts;

	std::string m_rootPath;

	double m_stepTime = 0.0;
};
}

#endif // CHALET_PROJECT_INITIALIZER_HPP
