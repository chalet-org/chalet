/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/ProjectInitializer.hpp"

#include "FileTemplates/StarterFileTemplates.hpp"

#include "State/AncillaryTools.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Diagnostic.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
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
	Color inputColor = Output::theme().alt;

	const auto& path = m_inputs.initPath();
	if (!Commands::pathExists(path))
	{
		if (Output::getUserInputYesNo(fmt::format("Directory '{}' does not exist. Create it?", path), true, inputColor))
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
	if (!Commands::pathIsEmpty(m_rootPath, { ".git", ".gitignore", "README.md", "LICENSE" }))
	{
		Diagnostic::error("Path '{}' is not empty. Please choose a different path, or clean this one first.", m_rootPath);
		return false;
	}

	Path::sanitize(m_rootPath);

	Output::print(Color::Reset, getBannerV2());

	BuildJsonProps props;
	props.workspaceName = String::getPathBaseName(m_rootPath);
	{
		auto& str = props.workspaceName;
		std::string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890-_";
		auto search = str.find_first_not_of(validChars.c_str());
		if (search != std::string::npos)
		{
			std::transform(str.begin(), str.end(), str.begin(), [&validChars](uchar c) -> uchar {
				return String::contains(c, validChars) ? c : '_';
			});
		}
	}
	props.projectName = props.workspaceName;
	props.version = "1.0.0";
	props.location = "src";

	std::string language = "C++";

	Output::getUserInput("Workspace name:", props.workspaceName, inputColor, "This should identify the entire workspace, as opposed to a build target");
	Output::getUserInput("Version:", props.version, inputColor, "The initial version of the application or library", [](std::string& input) {
		return input.find_first_not_of("1234567890.") == std::string::npos;
	});
	Output::getUserInput("Project target name:", props.projectName, inputColor, "Allowed characters: A-Z a-z 0-9 _+-.", [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
		return validChars && input.size() >= 3;
	});
#if defined(CHALET_MACOS)
	StringList allowedLangs{ "C++", "C", "Objective-C", "Objective-C++" };
#else
	StringList allowedLangs{ "C++", "C" };
#endif
	Output::getUserInput("Code language:", language, inputColor, fmt::format("Allowed values: {}", String::join(allowedLangs, " ")), [&allowedLangs](std::string& input) {
		return String::equals(allowedLangs, input);
	});

	StringList sourceExts;
	if (String::equals("C", language))
	{
		props.language = CodeLanguage::C;
		props.specialization = CxxSpecialization::C;
		props.langStandard = "17";
		sourceExts.emplace_back(".c");
	}
#if defined(CHALET_MACOS)
	else if (String::equals("Objective-C", language))
	{
		props.language = CodeLanguage::C;
		props.specialization = CxxSpecialization::ObjectiveC;
		props.langStandard = "17";
		sourceExts.emplace_back(".m");
	}
	else if (String::equals("Objective-C++", language))
	{
		props.language = CodeLanguage::CPlusPlus;
		props.specialization = CxxSpecialization::ObjectiveCPlusPlus;
		props.langStandard = "20";
		sourceExts.emplace_back(".mm");
	}
#endif
	else
	{
		props.language = CodeLanguage::CPlusPlus;
		props.specialization = CxxSpecialization::CPlusPlus;
		props.langStandard = "20";
		sourceExts.emplace_back(".cpp");
		sourceExts.emplace_back(".cxx");
		sourceExts.emplace_back(".cc");
	}

	bool isC = props.language == CodeLanguage::C;
	if (!isC)
	{
		Output::getUserInput("C++ Standard:", props.langStandard, inputColor, "Common choices: 20 17 14 11", [](std::string& input) {
			return RegexPatterns::matchesCxxStandardShort(input) || RegexPatterns::matchesGnuCppStandard(input);
		});

		if (props.langStandard.size() <= 2)
			props.langStandard = fmt::format("c++{}", props.langStandard);
	}
	else
	{
		Output::getUserInput("C Standard:", props.langStandard, inputColor, "Common choices: 17 11", [](std::string& input) {
			return RegexPatterns::matchesCxxStandardShort(input) || RegexPatterns::matchesGnuCStandard(input);
		});

		if (props.langStandard.size() <= 2)
			props.langStandard = fmt::format("c{}", props.langStandard);
	}

	props.useLocation = Output::getUserInputYesNo("Detect source files automatically?", true, inputColor, "If yes, 'location' is used, otherwise 'files' must be added explicitly");

	Output::getUserInput("Root source directory:", props.location, inputColor, "The primary location for source files", [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-") == std::string::npos;
		return validChars && input.size() >= 3;
	});

	props.mainSource = fmt::format("main{}", sourceExts.front());

	Output::getUserInput(fmt::format("Main source file:"), props.mainSource, inputColor, fmt::format("Must end in: {}", String::join(sourceExts, " ")), [&sourceExts, isC = isC](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = input.size() >= 3 && lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
		if (validChars && ((isC && !String::endsWith(sourceExts, input)) || (!isC && !String::endsWith(sourceExts, lower))))
		{
			input = String::getPathBaseName(input) + sourceExts.front();
		}
		return validChars;
	});

	if (props.specialization != CxxSpecialization::ObjectiveC)
	{
		if (Output::getUserInputYesNo("Use a precompiled header?", true, inputColor, "Precompiled headers are a way of reducing compile times"))
		{
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

			props.precompiledHeader = fmt::format("pch{}", headerExts.front());

			Output::getUserInput(fmt::format("Precompiled header:"), props.precompiledHeader, inputColor, fmt::format("Must end in: {}", String::join(headerExts, " ")), [&headerExts, isC = isC](std::string& input) {
				auto lower = String::toLowerCase(input);
				bool validChars = input.size() >= 3 && lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
				if (validChars && ((isC && !String::endsWith(headerExts, input)) || (!isC && !String::endsWith(headerExts, lower))))
				{
					input = String::getPathBaseName(input) + headerExts.front();
				}
				return validChars;
			});
		}
	}

	props.defaultConfigs = Output::getUserInputYesNo("Include default build configurations in build file?", false, inputColor, "Optional, but can be customized or restricted to certain configurations");
	props.envFile = Output::getUserInputYesNo("Include a .env file?", false, inputColor, "Optionally add environment variables or search paths to the build");
	props.makeGitRepository = Output::getUserInputYesNo("Initialize a git repository?", false, inputColor, "This will also create a .gitignore file");

	const std::string blankLine(80, ' ');
	std::cout << blankLine << std::flush;
	// Commands::sleep(0.5);

	Output::lineBreak();
	Output::printSeparator();

	double stepTime = 0.1;

	{
		Output::printInfo(fmt::format("{}/{}", props.location, props.mainSource));
		Output::lineBreak();

		auto mainCpp = StarterFileTemplates::getMainCxx(props.language, props.specialization);
		String::replaceAll(mainCpp, "\t", "   ");
		std::cout << Output::getAnsiStyle(Output::theme().build) << mainCpp << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::printSeparator();
	}

	if (!props.precompiledHeader.empty())
	{
		Output::printInfo(fmt::format("{}/{}", props.location, props.precompiledHeader));
		Output::lineBreak();

		const auto pch = StarterFileTemplates::getPch(props.precompiledHeader, props.language, props.specialization);
		std::cout << Output::getAnsiStyle(Output::theme().build) << pch << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::printSeparator();
	}

	if (props.makeGitRepository)
	{
		Output::printInfo(".gitignore");
		Output::lineBreak();

		const auto gitIgnore = StarterFileTemplates::getGitIgnore(m_inputs.outputDirectory(), m_inputs.settingsFile());
		std::cout << Output::getAnsiStyle(Output::theme().build) << gitIgnore << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::printSeparator();
	}

	if (props.envFile)
	{
#if defined(CHALET_WIN32)
		std::string env{ ".env.windows" };
#else
		std::string env{ ".env" };
#endif
		Output::printInfo(env);
		Output::lineBreak();

		const auto envFile = StarterFileTemplates::getDotEnv();
		std::cout << Output::getAnsiStyle(Output::theme().build) << envFile << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::printSeparator();
	}

	{
		Output::printInfo("chalet.json");
		Output::lineBreak();

		auto jsonFile = StarterFileTemplates::getBuildJson(props);
		std::cout << Output::getAnsiStyle(Output::theme().build) << jsonFile.dump(3, ' ') << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::printSeparator();
	}

	Output::lineBreak();

	if (Output::getUserInputYesNo("Does everything look okay?", true, inputColor))
	{
		return doRun(props);
	}
	else
	{
		Output::lineBreak();
		Output::displayStyledSymbol(Output::theme().info, " ", "Exiting...", false);
		Output::lineBreak();
		return false;
	}
}

/*****************************************************************************/
bool ProjectInitializer::doRun(const BuildJsonProps& inProps)
{
	Diagnostic::infoEllipsis("Initializing a new workspace called '{}'", inProps.workspaceName);
	// Commands::sleep(1.5);

	bool result = true;

	if (!makeBuildJson(inProps))
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

		if (Output::getUserInputYesNo("Run 'chalet configure'?", true, Output::theme().alt))
		{
			auto appPath = m_inputs.appPath();
			if (!Commands::pathExists(appPath))
			{
				appPath = Commands::which("chalet");
			}

			auto inputFile = fmt::format("{}/{}", m_rootPath, m_inputs.defaultInputFile());
			auto settingsFile = fmt::format("{}/{}", m_rootPath, m_inputs.defaultSettingsFile());
			auto outputDir = fmt::format("{}/{}", m_rootPath, m_inputs.defaultOutputDirectory());
			if (!Commands::subprocess({
					std::move(appPath),
					"configure",
					"--input-file",
					std::move(inputFile),
					"--settings-file",
					std::move(settingsFile),
					"--output-dir",
					std::move(outputDir),
				}))
				return false;
		}
		else
		{
			Output::lineBreak();
		}

		auto symbol = Unicode::diamond();
		Output::displayStyledSymbol(Output::theme().alt, symbol, "Happy coding!");

		Output::lineBreak();
	}
	else
	{
		Output::lineBreak();
		Diagnostic::error("There was an error creating the project files.");
	}

	return result;
}

/*****************************************************************************/
bool ProjectInitializer::makeBuildJson(const BuildJsonProps& inProps)
{
	const auto buildJsonPath = fmt::format("{}/chalet.json", m_rootPath);

	auto jsonFile = StarterFileTemplates::getBuildJson(inProps);

	JsonFile::saveToFile(jsonFile, buildJsonPath);

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::makeMainCpp(const BuildJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.mainSource);
	const auto contents = StarterFileTemplates::getMainCxx(inProps.language, inProps.specialization);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makePch(const BuildJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.precompiledHeader);
	const auto contents = StarterFileTemplates::getPch(inProps.precompiledHeader, inProps.language, inProps.specialization);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeGitIgnore()
{
	const auto outFile = fmt::format("{}/.gitignore", m_rootPath);
	const auto contents = StarterFileTemplates::getGitIgnore(m_inputs.outputDirectory(), m_inputs.settingsFile());

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeDotEnv()
{
#if defined(CHALET_WIN32)
	std::string env{ ".env.windows" };
#else
	std::string env{ ".env" };
#endif
	const auto outFile = fmt::format("{}/{}", m_rootPath, env);
	const auto contents = StarterFileTemplates::getDotEnv();

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
std::string ProjectInitializer::getBannerV1() const
{
	auto color = Output::getAnsiStyle(Color::Magenta);
	auto reset = Output::getAnsiReset();
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
	auto reset = Output::getAnsiReset();
	return fmt::format(R"art(
                                      {c1}./\.{reset}
                                   {c1}./J/''\L\.{reset}
                                {c1}./J/'{c3}======{c1}'\L\.{reset}
                             {c1}./J/' {c3}/'  ¦¦  '\ {c1}'\L\.{reset}
                          {c1}./J/'{c3}¦ '     ¦¦     ' ¦{c1} '\L\.{reset}
                       {c1}./J/'{c3}==¦¦================¦¦=={c1}'\L\.{reset}
                    {c1}./J/'{c3}¦¦   ¦¦ /'          '\ ¦¦   ¦¦{c1}'\L\.{reset}
                 {c1}./J/'{c3}   ¦¦===¦¦{c2}     {c1}CHALET{c3}     {c3}¦¦===¦¦   {c1}'\L\.{reset}
             {c1}./:''{c2}  |  U {c3}¦¦   ¦¦{c2}    ___  ___    {c3}¦¦   ¦¦{c2} U  |  {c1}'':\.{reset}
         {c1}./:''{c3}.{c2} O U | O  {c3}¦¦   ¦¦{c2}   |   ||   |   {c3}¦¦   ¦¦{c2}  O | U O {c3}.{c1}'':\.{reset}
             {c3}||{c2}  O  |  U {c3}MMMMMMM{c2}   |   ||o  |   {c3}MMMMMMM{c2} U  |  O  {c3}||{reset}
             {c3}||{c2}_____|____{c3}MMMMMMM{c2}___|___||___|___{c3}MMMMMMM{c2}____|_____{c3}||{reset}
)art",
		FMT_ARG(c1),
		FMT_ARG(c2),
		FMT_ARG(c3),
		FMT_ARG(reset));
}
}
