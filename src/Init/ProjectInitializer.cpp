/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/ProjectInitializer.hpp"

#include "FileTemplates/StarterFileTemplates.hpp"

#include "Core/CommandLineInputs.hpp"
#include "Init/ChaletJsonProps.hpp"
#include "Process/Environment.hpp"
#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Unicode.hpp"
#include "Utility/Path.hpp"
#include "Utility/RegexPatterns.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

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
	const auto& path = m_inputs.initPath();
	if (!Commands::pathExists(path))
	{
		if (Output::getUserInputYesNo(fmt::format("Directory '{}' does not exist. Create it?", path), true))
		{
			if (!Commands::makeDirectory(path))
			{
				Diagnostic::error("Error creating directory '{}'", path);
				return false;
			}
		}
		else
			return false;
	}

	// At the moment, only initialize an empty path
	m_rootPath = Commands::getCanonicalPath(path);
	m_stepTime = 0.1;

	if (!Commands::pathIsEmpty(m_rootPath, { ".git", ".gitignore", "README.md", "LICENSE" }))
	{
		Diagnostic::error("Path '{}' is not empty. Please choose a different path, or clean this one first.", m_rootPath);
		return false;
	}

	Path::unix(m_rootPath);

	auto initTemplate = m_inputs.initTemplate();
	if (initTemplate == InitTemplateType::Unknown)
	{
		Diagnostic::error("The specified initialization template was not recognized");
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

	bool isCmakeTemplate = initTemplate == InitTemplateType::CMake;

	bool result = false;
	ChaletJsonProps props;
	if (isCmakeTemplate)
	{
		result = initializeCMakeWorkspace(props);
	}
	else
	{
		result = initializeNormalWorkspace(props);
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
		Output::lineBreak();
		Output::displayStyledSymbol(Output::theme().info, " ", "Exiting...");
		Output::lineBreak();
		return false;
	}
}

/*****************************************************************************/
bool ProjectInitializer::initializeNormalWorkspace(ChaletJsonProps& outProps)
{
	outProps.workspaceName = getWorkspaceName();
	outProps.version = getWorkspaceVersion();
	outProps.projectName = getProjectName(outProps.workspaceName);

	auto language = getCodeLanguage();
	outProps.language = language;

	outProps.langStandard = getLanguageStandard(outProps.language);

#if defined(CHALET_WIN32) // modules can only be used in MSVC so far
	outProps.modules = getUseCxxModules(outProps.language, outProps.langStandard);
#endif
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
	outProps.makeGitRepository = getMakeGitRepository();

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

	printFileNameAndContents(true, "chalet.json", [&outProps]() {
		auto jsonFile = StarterFileTemplates::getStandardChaletJson(outProps);
		return jsonFile.dump(3, ' ');
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

	auto language = getCodeLanguage();
	outProps.language = language;

	outProps.langStandard = getLanguageStandard(outProps.language);

	m_sourceExts = getSourceExtensions(outProps.language, outProps.modules);

	outProps.useLocation = getUseLocation();
	outProps.location = getRootSourceDirectory();
	outProps.mainSource = getMainSourceFile(outProps.language);
	outProps.precompiledHeader = getCxxPrecompiledHeaderFile(outProps.language);
	// outProps.defaultConfigs = getIncludeDefaultBuildConfigurations();
	outProps.envFile = getMakeEnvFile();
	outProps.makeGitRepository = getMakeGitRepository();

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

	printFileNameAndContents(true, "chalet.json", [&outProps]() {
		auto jsonFile = StarterFileTemplates::getCMakeStarterChaletJson(outProps);
		return jsonFile.dump(3, ' ');
	});

	printFileNameAndContents(true, "CMakeLists.txt", [&outProps]() {
		return StarterFileTemplates::getCMakeStarter(outProps);
	});

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::doRun(const ChaletJsonProps& inProps)
{
	Diagnostic::infoEllipsis("Initializing a new workspace called '{}'", inProps.workspaceName);
	// Commands::sleep(1.5);

	bool result = true;

	if (!makeChaletJson(inProps))
		result = false;

	if (result)
	{
		auto location = fmt::format("{}/{}", m_rootPath, inProps.location); // src directory
		Commands::makeDirectory(location);

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

		if (m_inputs.initTemplate() == InitTemplateType::CMake)
		{
			if (!makeCMakeLists(inProps))
				result = false;
		}

		if (inProps.makeGitRepository)
		{
			auto git = AncillaryTools::getPathToGit();
			if (!git.empty())
			{
				if (!Commands::subprocess({ git, "-C", m_rootPath, "init", "--quiet" }))
					result = false;
				else if (!Commands::subprocess({ git, "-C", m_rootPath, "checkout", "-b", "main", "--quiet" }))
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
			auto appPath = m_inputs.appPath();
			if (!Commands::pathExists(appPath))
			{
				appPath = Commands::which("chalet");
			}

			if (!Commands::subprocess({ std::move(appPath), "configure" }, m_rootPath))
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
	const auto filePath = fmt::format("{}/chalet.json", m_rootPath);

	Json jsonFile;
	if (m_inputs.initTemplate() == InitTemplateType::CMake)
	{
		jsonFile = StarterFileTemplates::getCMakeStarterChaletJson(inProps);
	}
	else
	{
		jsonFile = StarterFileTemplates::getStandardChaletJson(inProps);
	}

	return JsonFile::saveToFile(jsonFile, filePath);
}

/*****************************************************************************/
bool ProjectInitializer::makeMainCpp(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.mainSource);
	const auto contents = StarterFileTemplates::getMainCxx(inProps.language, inProps.langStandard, inProps.modules);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makePch(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.precompiledHeader);
	const auto contents = StarterFileTemplates::getPch(inProps.precompiledHeader, inProps.language);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeCMakeLists(const ChaletJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/CMakeLists.txt", m_rootPath);
	const auto contents = StarterFileTemplates::getCMakeStarter(inProps);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeGitIgnore()
{
	const auto outFile = fmt::format("{}/.gitignore", m_rootPath);
	const auto contents = StarterFileTemplates::getGitIgnore(m_inputs.defaultOutputDirectory(), m_inputs.settingsFile());

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeDotEnv()
{
	const auto outFile = fmt::format("{}/{}", m_rootPath, m_inputs.platformEnv());
	const auto contents = StarterFileTemplates::getDotEnv();

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
std::string ProjectInitializer::getBannerV1() const
{
	auto& theme = Output::theme();
	auto color = Output::getAnsiStyle(theme.header);
	auto reset = Output::getAnsiStyle(theme.reset);
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
	auto c1 = Output::getAnsiStyle(theme.header);
	auto c2 = Output::getAnsiStyle(theme.flair);
	auto c3 = Output::getAnsiStyle(theme.info);
	auto reset = Output::getAnsiStyle(theme.reset);
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
std::string ProjectInitializer::getWorkspaceName() const
{
	std::string result = String::getPathBaseName(m_rootPath);

	auto onValidate = [](std::string& input) {
		std::string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_";
		auto search = input.find_first_not_of(validChars.c_str());
		if (search != std::string::npos)
		{
			std::transform(input.begin(), input.end(), input.begin(), [&validChars](uchar c) -> uchar {
				return String::contains(c, validChars) ? c : '_';
			});
		}
		return true;
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

	Output::getUserInput("Project target name:", result, "Allowed characters: A-Z a-z 0-9 _+-.", [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
		return validChars && input.size() >= 3;
	});

	return result;
}

/*****************************************************************************/
std::string ProjectInitializer::getRootSourceDirectory() const
{
	std::string result{ "src" };

	Output::getUserInput("Root source directory:", result, "The primary location for source files.", [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-") == std::string::npos;
		return validChars && input.size() >= 3;
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
	Output::getUserInput(fmt::format("Main source file:"), result, fmt::format("{}: {}", label, String::join(m_sourceExts, " ")), [this, isC = isC](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = input.size() >= 3 && lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
		if (validChars && (isC && !String::endsWith(m_sourceExts, input)))
		{
			input = String::getPathBaseName(input) + m_sourceExts.front();
		}
		return validChars;
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
			headerExts.emplace_back(".hxx");
			headerExts.emplace_back(".hh");
			headerExts.emplace_back(".h");
		}

		result = fmt::format("pch{}", headerExts.front());

		auto label = isC ? "Must end in" : "Recommended extensions";
		Output::getUserInput(fmt::format("Precompiled header file:"), result, fmt::format("{}: {}", label, String::join(headerExts, " ")), [&headerExts, isC = isC](std::string& input) {
			auto lower = String::toLowerCase(input);
			bool validChars = input.size() >= 3 && lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
			if (validChars && (isC && !String::endsWith(headerExts, input)))
			{
				input = String::getPathBaseName(input) + headerExts.front();
			}
			return validChars;
		});
	}

	return result;
}

/*****************************************************************************/
CodeLanguage ProjectInitializer::getCodeLanguage() const
{
	CodeLanguage ret = CodeLanguage::None;

#if defined(CHALET_MACOS)
	StringList allowedLangs{ "C++", "C", "Objective-C", "Objective-C++" };
#else
	StringList allowedLangs{ "C++", "C" };
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
bool ProjectInitializer::getUseCxxModules(const CodeLanguage inLang, std::string langStandard) const
{
	if (inLang != CodeLanguage::CPlusPlus)
		return false;

	String::replaceAll(langStandard, "gnu++", "");
	String::replaceAll(langStandard, "c++", "");

	if (!langStandard.empty() && langStandard.front() == '2')
	{
		return Output::getUserInputYesNo("Enable C++ modules?", false, "If true, C++ source files are treated as modules. (MSVC Only)");
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
	auto buildColor = Output::getAnsiStyle(Output::theme().build);
	auto reset = Output::getAnsiStyle(Output::theme().reset);

	std::cout.write(buildColor.data(), buildColor.size());
	std::cout.write(contents.data(), contents.size());
	std::cout.write(reset.data(), reset.size());
	std::cout.put('\n');
	std::cout.flush();

	Commands::sleep(m_stepTime);

	Output::lineBreak();
	Output::printSeparator();
}

/*****************************************************************************/
void ProjectInitializer::printUserInputSplit() const
{
	const std::string blankLine(80, ' ');
	std::cout.write(blankLine.data(), blankLine.size());
	std::cout.flush();
	// Commands::sleep(0.5);

	Output::lineBreak();
	Output::printSeparator();
}
}
