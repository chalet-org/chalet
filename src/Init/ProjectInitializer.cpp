/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/ProjectInitializer.hpp"

#include "FileTemplates/StarterFileTemplates.hpp"

#include "Terminal/Commands.hpp"
#include "Terminal/Diagnostic.hpp"
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
	const auto& path = m_inputs.initPath();
	if (!Commands::pathExists(path))
	{
		Diagnostic::error("Path '{}' does not exist. Please create it first.", path);
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

	Output::lineBreak();
	Output::print(Color::Reset, ".    `     .     .  `   ,    .    `    .   .    '       `    .   ,    '  .   ,  ");
	Output::print(Color::Reset, "    .     `    .   ,  '    .   ,   .         ,   .    '   `    .       .   .    ");
	Output::print(Color::Magenta, "                                   _▂▃▅▇▇▅▃▂_");
	Output::print(Color::Reset, "▂▂▂▂▃▃▂▃▅▃▂▃▅▃▂▂▂▃▃▂▂▃▅▅▃▂▂▂▃▃▂▂▃▅▃▂ CHALET ▂▃▅▃▂▂▃▃▂▂▂▃▅▅▃▂▂▃▃▂▂▂▃▅▃▂▃▅▃▂▃▃▂▂▂▂");
	Output::lineBreak();

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

	Color inputColor = Color::Magenta;

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
		props.langStandard = "c17";
		sourceExts.push_back(".c");
	}
#if defined(CHALET_MACOS)
	else if (String::equals("Objective-C", language))
	{
		props.language = CodeLanguage::C;
		props.specialization = CxxSpecialization::ObjectiveC;
		props.langStandard = "c17";
		sourceExts.push_back(".m");
	}
	else if (String::equals("Objective-C++", language))
	{
		props.language = CodeLanguage::CPlusPlus;
		props.specialization = CxxSpecialization::ObjectiveCPlusPlus;
		props.langStandard = "c++17";
		sourceExts.push_back(".mm");
	}
#endif
	else
	{
		props.language = CodeLanguage::CPlusPlus;
		props.specialization = CxxSpecialization::CPlusPlus;
		props.langStandard = "c++17";
		sourceExts.push_back(".cpp");
		sourceExts.push_back(".cxx");
		sourceExts.push_back(".cc");
	}

	bool isC = props.language == CodeLanguage::C;
	if (!isC)
	{
		Output::getUserInput("C++ Standard:", props.langStandard, inputColor, "Common choices: c++20 c++17 c++14 c++11", [](std::string& input) {
			return RegexPatterns::matchesGnuCppStandard(input);
		});
	}
	else
	{
		Output::getUserInput("C Standard:", props.langStandard, inputColor, "Common choices: c17 c11", [](std::string& input) {
			return RegexPatterns::matchesGnuCStandard(input);
		});
	}

	props.useLocation = Output::getUserInputYesNo("Detect source files automatically?", inputColor, "If yes, 'location' is used, otherwise 'files' must be added explicitly");

	Output::getUserInput("Root source directory:", props.location, inputColor, "The primary location for source files", [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-") == std::string::npos;
		return validChars && input.size() >= 3;
	});

	props.mainSource = fmt::format("main{}", sourceExts.front());

	Output::getUserInput(fmt::format("Main source file:"), props.mainSource, inputColor, fmt::format("Must end in: {}", String::join(sourceExts, " ")), [&sourceExts, isC = isC](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
		if (validChars && ((isC && !String::endsWith(sourceExts, input)) || (!isC && !String::endsWith(sourceExts, lower))))
		{
			input = String::getPathBaseName(input) + sourceExts.front();
		}
		return validChars && input.size() >= 3;
	});

	if (props.specialization != CxxSpecialization::ObjectiveC)
	{
		if (Output::getUserInputYesNo("Use a precompiled header?", inputColor, "Precompiled headers are known to reduce compile times"))
		{
			StringList headerExts;
			if (isC)
			{
				headerExts.push_back(".h");
			}
			else
			{
				headerExts.push_back(".hpp");
				headerExts.push_back(".hxx");
				headerExts.push_back(".hh");
				headerExts.push_back(".h");
			}

			props.precompiledHeader = fmt::format("pch{}", headerExts.front());

			Output::getUserInput(fmt::format("Precompiled header:"), props.precompiledHeader, inputColor, fmt::format("Must end in: {}", String::join(headerExts, " ")), [&headerExts, isC = isC](std::string& input) {
				auto lower = String::toLowerCase(input);
				bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_+-.") == std::string::npos;
				if (validChars && ((isC && !String::endsWith(headerExts, input)) || (!isC && !String::endsWith(headerExts, lower))))
				{
					input = String::getPathBaseName(input) + headerExts.front();
				}
				return validChars && input.size() >= 3;
			});
		}
	}

	props.defaultConfigs = Output::getUserInputYesNo("Include default build configurations in build file?", inputColor, "Optional, but can be customized or restricted to certain configurations");
	props.envFile = Output::getUserInputYesNo("Include a .env file?", inputColor, "Optionally add environment variables or search paths to the build");
	props.makeGitRepository = Output::getUserInputYesNo("Initialize a git repository?", inputColor, "This will also create a .gitignore file");

	const std::string blankLine(80, ' ');
	std::cout << blankLine << std::flush;
	// Commands::sleep(0.5);

	const std::string separator(80, '-');

	Output::lineBreak();
	Output::print(Color::Black, separator);

	double stepTime = 0.1;

	{
		Output::print(Color::Reset, fmt::format("{}/{}", props.location, props.mainSource));
		Output::lineBreak();

		auto mainCpp = StarterFileTemplates::getMainCxx(props.language, props.specialization);
		String::replaceAll(mainCpp, "\t", "   ");
		std::cout << Output::getAnsiStyle(Color::Blue) << mainCpp << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	if (!props.precompiledHeader.empty())
	{
		Output::print(Color::Reset, fmt::format("{}/{}", props.location, props.precompiledHeader));
		Output::lineBreak();

		const auto pch = StarterFileTemplates::getPch(props.precompiledHeader, props.language, props.specialization);
		std::cout << Output::getAnsiStyle(Color::Blue) << pch << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	if (props.makeGitRepository)
	{
		Output::print(Color::Reset, ".gitignore");
		Output::lineBreak();

		const auto gitIgnore = StarterFileTemplates::getGitIgnore(m_inputs.buildPath());
		std::cout << Output::getAnsiStyle(Color::Blue) << gitIgnore << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	if (props.envFile)
	{
#if defined(CHALET_WIN32)
		std::string env{ ".env.windows" };
#else
		std::string env{ ".env" };
#endif
		Output::print(Color::Reset, env);
		Output::lineBreak();

		const auto envFile = StarterFileTemplates::getDotEnv();
		std::cout << Output::getAnsiStyle(Color::Blue) << envFile << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	{
		Output::print(Color::Reset, "build.json");
		Output::lineBreak();

		auto jsonFile = StarterFileTemplates::getBuildJson(props);
		std::cout << Output::getAnsiStyle(Color::Blue) << jsonFile.dump(3, ' ') << Output::getAnsiReset() << std::endl;

		Commands::sleep(stepTime);

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	Output::lineBreak();

	if (Output::getUserInputYesNo("Does everything look okay?", inputColor))
	{
		return doRun(props);
	}
	else
	{
		Output::lineBreak();
		Output::displayStyledSymbol(Color::Reset, " ", "Exiting...", false);
		Output::lineBreak();
		return false;
	}
}

/*****************************************************************************/
bool ProjectInitializer::doRun(const BuildJsonProps& inProps)
{
	Diagnostic::info(fmt::format("Initializing a new workspace called '{}'", inProps.workspaceName), false);
	// Commands::sleep(1.5);

	bool result = true;

	if (!makeBuildJson(inProps))
		result = false;

	if (result)
	{
		Commands::makeDirectory(fmt::format("{}/{}", m_rootPath, inProps.location));

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
			auto git = Commands::which("git");
			if (!git.empty())
			{
				if (!Commands::subprocess({ git, "init", "--quiet" }))
					result = false;
				else if (!Commands::subprocess({ git, "checkout", "-b", "main", "--quiet" }))
					result = false;
			}
		}
	}

	if (result)
	{
		Diagnostic::printDone();
		// Output::lineBreak();

		if (Output::getUserInputYesNo("Run 'chalet configure'?", Color::Magenta))
		{
			if (!String::equals('.', m_inputs.initPath())) // TODO: init > configure from parent dir
				return true;

			auto appPath = m_inputs.appPath();
			if (!Commands::pathExists(appPath))
			{
				appPath = Commands::which("chalet");
			}

			if (!Commands::subprocess({ std::move(appPath), "configure" }))
				return false;
		}
		else
		{
			Output::lineBreak();
		}

		auto symbol = Unicode::diamond();
		Output::displayStyledSymbol(Color::Magenta, symbol, "Happy coding!");

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
	const auto buildJsonPath = fmt::format("{}/build.json", m_rootPath);

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
	const auto contents = StarterFileTemplates::getGitIgnore(m_inputs.buildPath());

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
}
