/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/ProjectInitializer.hpp"

#include "FileTemplates/StarterFileTemplates.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Init/ChaletJsonProps.hpp"
#include "Process/Environment.hpp"
#include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "System/Files.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/Path.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Yaml/YamlFile.hpp"
#include "Json/JsonFile.hpp"

#if defined(CHALET_MSVC)
#else
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wtype-limits"
#endif

namespace chalet
{
/*****************************************************************************/
ProjectInitializer::ProjectInitializer(const CommandLineInputs& inInputs) :
	m_inputs(inInputs)
{
}

/*****************************************************************************/
bool ProjectInitializer::run()
{
	CHALET_TRY
	{
		const auto& path = m_inputs.initPath();
		if (!Files::pathExists(path))
		{
			if (Output::getUserInputYesNo(fmt::format("Directory '{}' does not exist. Create it?", path), true))
			{
				if (!Files::makeDirectory(path))
				{
					Diagnostic::error("Error creating directory '{}'", path);
					return false;
				}
			}
			else
				return false;
		}

		// At the moment, only initialize an empty path
		m_rootPath = Files::getCanonicalPath(path);
		m_stepTime = 0.1;

		if (!Files::pathIsEmpty(m_rootPath, { ".git", ".gitignore", "README.md", "LICENSE" }))
		{
			Diagnostic::error("Path '{}' is not empty. Please choose a different path, or clean this one first.", m_rootPath);
			return false;
		}

		Path::toUnix(m_rootPath);

		auto initTemplate = m_inputs.initTemplate();
		if (initTemplate == InitTemplateType::Unknown)
		{
			Diagnostic::error("The specified project template was not recognized");
			return false;
		}

		{
			/*auto banner = String::split(getBannerV2(), '\n');
		for (auto& line : banner)
		{
			std::cout.write(line.data(), line.size());
			std::cout.put('\n');
			std::cout.flush();
		}*/
			auto banner = getBannerV2();
			std::cout.write(banner.data(), banner.size());
			std::cout.put('\n');
			std::cout.flush();
		}

		bool result = false;
		ChaletJsonProps props;

		switch (initTemplate)
		{
			case InitTemplateType::CMake:
				result = initializeCMakeWorkspace(props);
				break;
			case InitTemplateType::Meson:
				result = initializeMesonWorkspace(props);
				break;
			case InitTemplateType::None:
			default:
				result = initializeNormalWorkspace(props);
				break;
		}

		if (!result)
			return false;

		Output::lineBreak();

		if (Output::getUserInputYesNo("Does everything look okay?", true))
		{
			return doRun(props);
		}
		else
		{
			showExit();
			return false;
		}
	}
	CHALET_CATCH(const std::exception& err)
	{
		if (::strcmp(err.what(), "SIGINT") != 0)
		{
			Diagnostic::error("Uncaught exception: {}", err.what());
		}
		return false;
	}
}

/*****************************************************************************/
void ProjectInitializer::showExit()
{
	Output::lineBreak();
	Output::displayStyledSymbol(Output::theme().info, " ", "Exiting...");
	Output::lineBreak();
}

/*****************************************************************************/
bool ProjectInitializer::initializeNormalWorkspace(ChaletJsonProps& outProps)
{
	outProps.workspaceName = getWorkspaceName();
	outProps.version = getWorkspaceVersion();
	outProps.projectName = getProjectName(outProps.workspaceName);
	outProps.language = getCodeLanguage();
	outProps.langStandard = getLanguageStandard(outProps.language);

	outProps.modules = getUseCxxModules(outProps.language, outProps.langStandard);
	m_sourceExts = getSourceExtensions(outProps.language, outProps.modules);

	outProps.useLocation = getUseLocation();
	outProps.location = getRootSourceDirectory();
	outProps.mainSource = getMainSourceFile(outProps.language);

	if (!outProps.modules)
	{
		outProps.precompiledHeader = getCxxPrecompiledHeaderFile(outProps.language);
	}

	outProps.defaultConfigs = getIncludeDefaultBuildConfigurations();
	outProps.envFile = getMakeEnvFile();
	outProps.inputFile = getInputFileFormat();
	outProps.makeGitRepository = getMakeGitRepository();

	outProps.isYaml = String::endsWith(".yaml", outProps.inputFile);

	printUserInputSplit();

	printFileNameAndContents(true, fmt::format("{}/{}", outProps.location, outProps.mainSource), [&outProps]() {
		auto mainCpp = StarterFileTemplates::getMainCxx(outProps.language, outProps.langStandard, outProps.modules);
		String::replaceAll(mainCpp, '\t', "   ");
		return mainCpp;
	});

	printFileNameAndContents(!outProps.precompiledHeader.empty(), fmt::format("{}/{}", outProps.location, outProps.precompiledHeader), [&outProps]() {
		return StarterFileTemplates::getPch(outProps.precompiledHeader, outProps.language);
	});

	printFileNameAndContents(outProps.makeGitRepository, ".gitignore", [this]() {
		return StarterFileTemplates::getGitIgnore(m_inputs.defaultOutputDirectory(), m_inputs.settingsFile());
	});

	printFileNameAndContents(outProps.envFile, m_inputs.platformEnv(), []() {
		return StarterFileTemplates::getDotEnv();
	});

	printFileNameAndContents(true, outProps.inputFile, [&outProps]() {
		auto jsonFile = StarterFileTemplates::getStandardChaletJson(outProps);
		if (outProps.isYaml)
			return YamlFile::asString(jsonFile);
		else
			return json::dump(jsonFile, 3, ' ');
	});

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::initializeCMakeWorkspace(ChaletJsonProps& outProps)
{
	Diagnostic::info("Template: CMake");

	outProps.workspaceName = getWorkspaceName();
	outProps.version = getWorkspaceVersion();
	outProps.projectName = getProjectName(outProps.workspaceName);
	outProps.language = getCodeLanguage();
	outProps.langStandard = getLanguageStandard(outProps.language);

	m_sourceExts = getSourceExtensions(outProps.language, outProps.modules);

	outProps.useLocation = getUseLocation();
	outProps.location = getRootSourceDirectory();
	outProps.mainSource = getMainSourceFile(outProps.language);
	outProps.precompiledHeader = getCxxPrecompiledHeaderFile(outProps.language);
	// outProps.defaultConfigs = getIncludeDefaultBuildConfigurations();
	outProps.envFile = getMakeEnvFile();
	outProps.inputFile = getInputFileFormat();
	outProps.makeGitRepository = getMakeGitRepository();

	outProps.isYaml = String::endsWith(".yaml", outProps.inputFile);

	printUserInputSplit();

	printFileNameAndContents(true, fmt::format("{}/{}", outProps.location, outProps.mainSource), [&outProps]() {
		auto mainCpp = StarterFileTemplates::getMainCxx(outProps.language, outProps.langStandard, outProps.modules);
		String::replaceAll(mainCpp, '\t', "   ");
		return mainCpp;
	});

	printFileNameAndContents(!outProps.precompiledHeader.empty(), fmt::format("{}/{}", outProps.location, outProps.precompiledHeader), [&outProps]() {
		return StarterFileTemplates::getPch(outProps.precompiledHeader, outProps.language);
	});

	printFileNameAndContents(outProps.makeGitRepository, ".gitignore", [this]() {
		return StarterFileTemplates::getGitIgnore(m_inputs.defaultOutputDirectory(), m_inputs.settingsFile());
	});

	printFileNameAndContents(outProps.envFile, m_inputs.platformEnv(), []() {
		return StarterFileTemplates::getDotEnv();
	});

	printFileNameAndContents(true, outProps.inputFile, [&outProps]() {
		auto jsonFile = StarterFileTemplates::getCMakeStarterChaletJson(outProps);
		if (outProps.isYaml)
			return YamlFile::asString(jsonFile);
		else
			return json::dump(jsonFile, 3, ' ');
	});

	printFileNameAndContents(true, "CMakeLists.txt", [&outProps]() {
		return StarterFileTemplates::getCMakeStarter(outProps);
	});

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::initializeMesonWorkspace(ChaletJsonProps& outProps)
{
	Diagnostic::info("Template: Meson");

	outProps.workspaceName = getWorkspaceName();
	outProps.version = getWorkspaceVersion();
	outProps.projectName = getProjectName(outProps.workspaceName);
	outProps.language = getCodeLanguage();
	outProps.langStandard = getLanguageStandard(outProps.language);

	m_sourceExts = getSourceExtensions(outProps.language, outProps.modules);

	outProps.useLocation = false;
	outProps.location = getRootSourceDirectory();
	outProps.mainSource = getMainSourceFile(outProps.language);
	outProps.precompiledHeader = getCxxPrecompiledHeaderFile(outProps.language);
	// outProps.defaultConfigs = getIncludeDefaultBuildConfigurations();
	outProps.envFile = getMakeEnvFile();
	outProps.inputFile = getInputFileFormat();
	outProps.makeGitRepository = getMakeGitRepository();

	outProps.isYaml = String::endsWith(".yaml", outProps.inputFile);

	printUserInputSplit();

	printFileNameAndContents(true, fmt::format("{}/{}", outProps.location, outProps.mainSource), [&outProps]() {
		auto mainCpp = StarterFileTemplates::getMainCxx(outProps.language, outProps.langStandard, outProps.modules);
		String::replaceAll(mainCpp, '\t', "   ");
		return mainCpp;
	});

	printFileNameAndContents(!outProps.precompiledHeader.empty(), fmt::format("{}/{}", outProps.location, outProps.precompiledHeader), [&outProps]() {
		return StarterFileTemplates::getPch(outProps.precompiledHeader, outProps.language);
	});

	printFileNameAndContents(outProps.makeGitRepository, ".gitignore", [this]() {
		return StarterFileTemplates::getGitIgnore(m_inputs.defaultOutputDirectory(), m_inputs.settingsFile());
	});

	printFileNameAndContents(outProps.envFile, m_inputs.platformEnv(), []() {
		return StarterFileTemplates::getDotEnv();
	});

	printFileNameAndContents(true, outProps.inputFile, [&outProps]() {
		auto jsonFile = StarterFileTemplates::getMesonStarterChaletJson(outProps);
		if (outProps.isYaml)
			return YamlFile::asString(jsonFile);
		else
			return json::dump(jsonFile, 3, ' ');
	});

	printFileNameAndContents(true, "meson.build", [&outProps]() {
		return StarterFileTemplates::getMesonStarter(outProps);
	});

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::doRun(const ChaletJsonProps& inProps)
{
	Diagnostic::infoEllipsis("Initializing a new workspace called '{}'", inProps.workspaceName);
	// Files::sleep(1.5);

	bool result = true;

	if (!makeChaletJson(inProps))
		result = false;

	if (result)
	{
		auto location = fmt::format("{}/{}", m_rootPath, inProps.location); // src directory
		Files::makeDirectory(location);

		if (!makeMainCpp(inProps))
			result = false;

		if (!inProps.precompiledHeader.empty())
		{
			if (!makePch(inProps))
				result = false;
		}

		if (inProps.makeGitRepository)
		{
			if (!makeGitIgnore())
				result = false;
		}

		if (inProps.envFile)
		{
			if (!makeDotEnv())
				result = false;
		}

		auto initTemplate = m_inputs.initTemplate();
		switch (initTemplate)
		{
			case InitTemplateType::CMake:
				if (!makeCMakeLists(inProps))
					result = false;
				break;
			case InitTemplateType::Meson:
				if (!makeMesonBuild(inProps))
					result = false;
				break;
			default:
				break;
		}

		if (inProps.makeGitRepository)
		{
			auto git = AncillaryTools::getPathToGit();
			if (!git.empty())
			{
				if (!Process::run({ git, "-C", m_rootPath, "init", "--quiet" }))
					result = false;
				else if (!Process::run({ git, "-C", m_rootPath, "checkout", "-b", "main", "--quiet" }))
					result = false;
			}
			else
			{
				Diagnostic::warn("A git repository was not be created, because git was not found.");
			}
		}
	}

	if (result)
	{
		Diagnostic::printDone();
		// Output::lineBreak();

		if (Output::getUserInputYesNo("Run 'chalet configure'?", true))
		{
			if (!Process::run({ m_inputs.appPath(), "configure" }, m_rootPath))
				return false;
		}
		else
		{
			Output::lineBreak();
		}

		auto symbol = Unicode::diamond();
		Output::displayStyledSymbol(Output::theme().note, symbol, "Happy coding!");

		Output::lineBreak();
	}
	else
	{
		Diagnostic::error("There was an error creating the project files.");
	}

	return result;
}

/*****************************************************************************/
bool ProjectInitializer::makeChaletJson(const ChaletJsonProps& inProps)
{
	const auto filePath = fmt::format("{}/{}", m_rootPath, inProps.inputFile);

	Json jsonFile;
	auto initTemplate = m_inputs.initTemplate();
	switch (initTemplate)
	{
		case InitTemplateType::CMake:
			jsonFile = StarterFileTemplates::getCMakeStarterChaletJson(inProps);
			break;
		case InitTemplateType::Meson:
			jsonFile = StarterFileTemplates::getMesonStarterChaletJson(inProps);
			break;
		default:
			jsonFile = StarterFileTemplates::getStandardChaletJson(inProps);
			break;
	}

	if (inProps.isYaml)
		return YamlFile::saveToFile(jsonFile, filePath);
	else
		return JsonFile::saveToFile(jsonFile, filePath);
}

/*****************************************************************************/
bool ProjectInitializer::makeMainCpp(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.mainSource);
	const auto contents = StarterFileTemplates::getMainCxx(inProps.language, inProps.langStandard, inProps.modules);

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makePch(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.precompiledHeader);
	const auto contents = StarterFileTemplates::getPch(inProps.precompiledHeader, inProps.language);

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeCMakeLists(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/CMakeLists.txt", m_rootPath);
	const auto contents = StarterFileTemplates::getCMakeStarter(inProps);

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeMesonBuild(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/meson.build", m_rootPath);
	const auto contents = StarterFileTemplates::getMesonStarter(inProps);

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeGitIgnore()
{
	const auto outFile = fmt::format("{}/.gitignore", m_rootPath);
	const auto contents = StarterFileTemplates::getGitIgnore(m_inputs.defaultOutputDirectory(), m_inputs.settingsFile());

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeDotEnv()
{
	const auto outFile = fmt::format("{}/{}", m_rootPath, m_inputs.platformEnv());
	const auto contents = StarterFileTemplates::getDotEnv();

	return Files::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
std::string ProjectInitializer::getBannerV1() const
{
	auto& theme = Output::theme();
	const auto& color = Output::getAnsiStyle(theme.header);
	const auto& reset = Output::getAnsiStyle(theme.reset);
	return fmt::format(R"art(
.    `     .     .  `   ,    .    `    .   .    '       `    .   ,    '  .   ,
    .     `    .   ,  '    .   ,   .         ,   .    '   `    .       .   .
                                   {color}_▂▃▅▇▇▅▃▂_{reset}
▂▂▂▂▃▃▂▃▅▃▂▃▅▃▂▂▂▃▃▂▂▃▅▅▃▂▂▂▃▃▂▂▃▅▃▂ CHALET ▂▃▅▃▂▂▃▃▂▂▂▃▅▅▃▂▂▃▃▂▂▂▃▅▃▂▃▅▃▂▃▃▂▂▂▂
)art",
		FMT_ARG(color),
		FMT_ARG(reset));
}

/*****************************************************************************/
std::string ProjectInitializer::getBannerV2() const
{
	auto& theme = Output::theme();
	const auto& c1 = Output::getAnsiStyle(theme.header);
	const auto& c2 = Output::getAnsiStyle(theme.flair);
	const auto& c3 = Output::getAnsiStyle(theme.info);
	const auto& reset = Output::getAnsiStyle(theme.reset);
	return fmt::format(R"art(
                                      {c1}./\.{reset}
                                   {c1}./J/''\L\.{reset}
                                {c1}./J/'{c3}======{c1}'\L\.{reset}
                             {c1}./J/' {c3}/'  ¦¦  '\ {c1}'\L\.{reset}
                          {c1}./J/'{c3}¦ '     ¦¦     ' ¦{c1} '\L\.{reset}
                       {c1}./J/'{c3}==¦¦================¦¦=={c1}'\L\.{reset}
                    {c1}./J/'{c3}¦¦   ¦¦ /'          '\ ¦¦   ¦¦{c1}'\L\.{reset}
                 {c1}./J/'{c3}   ¦¦===¦¦{c2}     {c3}{c1}CHALET{c3}     {c3}¦¦===¦¦   {c1}'\L\.{reset}
             {c1}./:''{c2}  |  U {c3}¦¦   ¦¦{c2}    ___  ___    {c3}¦¦   ¦¦{c2} U  |  {c3}{c1}'':\.{reset}
         {c1}./:''{c3}.{c2} O U | O  {c3}¦¦   ¦¦{c2}   |   ||   |   {c3}¦¦   ¦¦{c2}  O | U O {c3}.{c1}'':\.{reset}
             {c3}||{c2}  O  |  U {c3}MMMMMMM{c2}   |   ||o  |   {c3}MMMMMMM{c2} U  |  O  {c3}||{reset}
             {c3}||{c2}_____|____{c3}MMMMMMM{c2}___|___||___|___{c3}MMMMMMM{c2}____|_____{c3}||{reset}
)art",
		FMT_ARG(c1),
		FMT_ARG(c2),
		FMT_ARG(c3),
		FMT_ARG(reset));
}

/*****************************************************************************/
bool ProjectInitializer::checkForInvalidPathCharacters(std::string& input) const
{
	if (input.empty())
		return false;

	if (input.front() == '.' || input.back() == '.')
		return false;

	std::string invalidChars = "<>:\"/\\|?*";
	auto search = input.find_first_of(invalidChars.c_str());
	if (search != std::string::npos)
	{
		for (auto c : input)
		{
			if ((c >= 0 && c < 32) || String::contains(c, invalidChars))
				return false;
		}
	}
	return true;
}

/*****************************************************************************/
bool ProjectInitializer::checkForInvalidTargetNameCharacters(std::string& input) const
{
	if (input.empty())
		return false;

	if (input.front() == '.' || input.back() == '.')
		return false;

	std::string invalidChars = "<>:\"/\\|?*{}$";
	auto search = input.find_first_of(invalidChars.c_str());
	if (search != std::string::npos)
	{
		for (auto c : input)
		{
			if ((c >= 0 && c < 32) || String::contains(c, invalidChars))
				return false;
		}
	}
	return input.size() >= 2;
}

/*****************************************************************************/
void ProjectInitializer::ensureFileExtension(std::string& input, const StringList& inExts, const CodeLanguage inLang) const
{
	const bool isC = inLang == CodeLanguage::C;
	if ((isC && !String::endsWith(inExts, input)) || input.find('.') == std::string::npos)
	{
		input = String::getPathBaseName(input) + inExts.front();
	}
}

/*****************************************************************************/
std::string ProjectInitializer::getWorkspaceName() const
{
	std::string result = String::getPathBaseName(m_rootPath);

	auto onValidate = [this](std::string& input) {
		return checkForInvalidPathCharacters(input);
	};

	onValidate(result);

	Output::getUserInput("Workspace name:", result, "This should identify the entire workspace.", onValidate);

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getWorkspaceVersion() const
{
	std::string result{ "1.0.0" };

	Output::getUserInput("Version:", result, "The initial version of the workspace.", [](std::string& input) {
		return input.find_first_not_of("1234567890.") == std::string::npos;
	});

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getProjectName(const std::string& inWorkspaceName) const
{
	std::string result = inWorkspaceName;

	Output::getUserInput("Project target name:", result, "The name of the executable", [this](std::string& input) {
		return checkForInvalidTargetNameCharacters(input);
	});

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getRootSourceDirectory() const
{
	std::string result{ "src" };

	Output::getUserInput("Root source directory:", result, "The primary location for source files.", [this](std::string& input) {
		return checkForInvalidPathCharacters(input);
	});

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getMainSourceFile(const CodeLanguage inLang) const
{
	chalet_assert(!m_sourceExts.empty(), "No source extensions have been populated");

	std::string result = fmt::format("main{}", m_sourceExts.front());

	const bool isC = inLang == CodeLanguage::C;
	auto label = isC ? "Must end in" : "Recommended extensions";
	Output::getUserInput(fmt::format("Main source file:"), result, fmt::format("{}: {}", label, String::join(m_sourceExts, " ")), [this, inLang](std::string& input) {
		if (!checkForInvalidPathCharacters(input))
			return false;

		ensureFileExtension(input, m_sourceExts, inLang);
		return true;
	});

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getCxxPrecompiledHeaderFile(const CodeLanguage inLang) const
{
	std::string result;

	if (Output::getUserInputYesNo("Use a precompiled header?", true, "Precompiled headers are a way of reducing compile times"))
	{
		const bool isC = inLang == CodeLanguage::C || inLang == CodeLanguage::ObjectiveC;
		StringList headerExts;
		if (isC)
		{
			headerExts.emplace_back(".h");
		}
		else
		{
			headerExts.emplace_back(".hpp");
			headerExts.emplace_back(".h");
			headerExts.emplace_back(".hxx");
			headerExts.emplace_back(".hh");
		}

		result = fmt::format("pch{}", headerExts.front());

		auto label = isC ? "Must end in" : "Recommended extensions";
		Output::getUserInput(fmt::format("Precompiled header file:"), result, fmt::format("{}: {}", label, String::join(headerExts, " ")), [this, &headerExts, inLang](std::string& input) {
			if (!checkForInvalidPathCharacters(input))
				return false;

			ensureFileExtension(input, headerExts, inLang);
			return true;
		});
	}

	return result;
}

/*****************************************************************************/
CodeLanguage ProjectInitializer::getCodeLanguage() const
{
	CodeLanguage ret = CodeLanguage::None;

	StringList allowedLangs{ "C++", "C" };

#if defined(CHALET_MACOS)
	allowedLangs.emplace_back("Objective-C");
	allowedLangs.emplace_back("Objective-C++");
#endif
	std::string language = allowedLangs.front();

	Output::getUserInput("Code language:", language, fmt::format("Allowed values: {}", String::join(allowedLangs, " ")), [&allowedLangs](std::string& input) {
		return String::equals(allowedLangs, input);
	});

	if (String::equals("C", language))
	{
		ret = CodeLanguage::C;
	}
#if defined(CHALET_MACOS)
	else if (String::equals("Objective-C", language))
	{
		ret = CodeLanguage::ObjectiveC;
	}
	else if (String::equals("Objective-C++", language))
	{
		ret = CodeLanguage::ObjectiveCPlusPlus;
	}
#endif
	else
	{
		ret = CodeLanguage::CPlusPlus;
	}

	return ret;
}

/*****************************************************************************/
StringList ProjectInitializer::getSourceExtensions(const CodeLanguage inLang, const bool inModules) const
{
	StringList ret;
	if (inLang == CodeLanguage::C)
	{
		ret.emplace_back(".c");
	}
#if defined(CHALET_MACOS)
	else if (inLang == CodeLanguage::ObjectiveCPlusPlus)
	{
		ret.emplace_back(".mm");
	}
	else if (inLang == CodeLanguage::ObjectiveC)
	{
		ret.emplace_back(".m");
	}
#endif
	else
	{
		if (inModules)
		{
			ret.emplace_back(".cc");
			ret.emplace_back(".ixx");
		}

		ret.emplace_back(".cpp");

		if (!inModules)
			ret.emplace_back(".cc");

		ret.emplace_back(".cxx");
	}

	return ret;
}

/*****************************************************************************/
std::string ProjectInitializer::getLanguageStandard(const CodeLanguage inLang) const
{
	std::string ret;

	if (inLang == CodeLanguage::CPlusPlus || inLang == CodeLanguage::ObjectiveCPlusPlus)
	{
		ret = "20";
		Output::getUserInput("C++ Standard:", ret, "Common choices: 23 20 17 14 11", [](std::string& input) {
			return RegexPatterns::matchesCxxStandardShort(input) || RegexPatterns::matchesGnuCppStandard(input);
		});
	}
	else
	{
		ret = "17";
		Output::getUserInput("C Standard:", ret, "Common choices: 23 17 11", [](std::string& input) {
			return RegexPatterns::matchesCxxStandardShort(input) || RegexPatterns::matchesGnuCStandard(input);
		});
	}

	return ret;
}

/*****************************************************************************/
std::string ProjectInitializer::getInputFileFormat() const
{
	std::string ret = "json";
	auto presets = m_inputs.getConvertFormatPresets();
	auto description = fmt::format("Available formats: {}", String::join(presets));
	Output::getUserInput("Build file format:", ret, description, [](std::string& input) {
		return String::equals("json", input) || String::equals("yaml", input);
	});

	if (String::equals("yaml", ret))
		ret = m_inputs.yamlInputFile();
	else
		ret = m_inputs.defaultInputFile();

	return ret;
}

/*****************************************************************************/
bool ProjectInitializer::getUseCxxModules(const CodeLanguage inLang, std::string langStandard) const
{
	if (inLang != CodeLanguage::CPlusPlus)
		return false;

	String::replaceAll(langStandard, "gnu++", "");
	String::replaceAll(langStandard, "c++", "");

	if (!langStandard.empty() && langStandard.front() == '2')
	{
		return Output::getUserInputYesNo("Enable C++ modules?", false, "If true, C++ source files are treated as modules.");
	}

	return false;
}

/*****************************************************************************/
bool ProjectInitializer::getUseLocation() const
{
	return Output::getUserInputYesNo("Detect source files automatically?", true, "If yes, sources are globbed, otherwise they must be managed explicitly.");
}

/*****************************************************************************/
bool ProjectInitializer::getIncludeDefaultBuildConfigurations() const
{
	return Output::getUserInputYesNo("Include default build configurations in build file?", false, "Optional, but can be customized or restricted to certain configurations.");
}

/*****************************************************************************/
bool ProjectInitializer::getMakeEnvFile() const
{
	return Output::getUserInputYesNo("Include a .env file?", false, "Optionally add environment variables or search paths to the build.");
}

/*****************************************************************************/
bool ProjectInitializer::getMakeGitRepository() const
{
	return Output::getUserInputYesNo("Initialize a git repository?", false, "This will also create a .gitignore file.");
}

/*****************************************************************************/
void ProjectInitializer::printFileNameAndContents(const bool inCondition, const std::string& inFileName, const std::function<std::string()>& inGetContents) const
{
	if (!inCondition)
		return;

	Output::printInfo(inFileName);
	Output::lineBreak();

	std::string contents = inGetContents();
	const auto& buildColor = Output::getAnsiStyle(Output::theme().build);
	const auto& reset = Output::getAnsiStyle(Output::theme().reset);

	std::cout.write(buildColor.data(), buildColor.size());
	std::cout.write(contents.data(), contents.size());
	std::cout.write(reset.data(), reset.size());
	std::cout.put('\n');
	std::cout.flush();

	Files::sleep(m_stepTime);

	Output::lineBreak();
	Output::printSeparator();
}

/*****************************************************************************/
void ProjectInitializer::printUserInputSplit() const
{
	const std::string blankLine(80, ' ');
	std::cout.write(blankLine.data(), blankLine.size());
	std::cout.flush();
	// Files::sleep(0.5);

	Output::lineBreak();
	Output::printSeparator();
}
}

#if defined(CHALET_MSVC)
#else
	#pragma GCC diagnostic pop
#endif
