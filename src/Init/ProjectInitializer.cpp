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

	std::string separator{ "--------------------------------------------------------------------------------" };

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

	Output::getUserInput("Workspace name:", props.workspaceName, inputColor);
	Output::getUserInput("Version:", props.version, inputColor, [](std::string& input) {
		return input.find_first_not_of("1234567890.") == std::string::npos;
	});
	Output::getUserInput("Project target name:", props.projectName, inputColor, [](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz-_") == std::string::npos;
		return validChars;
	});
	Output::getUserInput("Code language:", language, inputColor, [](std::string& input) {
		return String::equals({ "C", "C++" }, input);
	});

	StringList sourceExts;
	if (String::equals("C", language))
	{
		props.language = CodeLanguage::C;
		props.langStandard = "c17";
		sourceExts.push_back(".c");
		Output::getUserInput("C Standard:", props.langStandard, inputColor, [](std::string& input) {
			return RegexPatterns::matchesGnuCStandard(input);
		});
	}
	else
	{
		props.language = CodeLanguage::CPlusPlus;
		props.langStandard = "c++17";
		sourceExts.push_back(".cpp");
		sourceExts.push_back(".cc");
		Output::getUserInput("C++ Standard:", props.langStandard, inputColor, [](std::string& input) {
			return RegexPatterns::matchesGnuCppStandard(input);
		});
	}
	props.mainSource = fmt::format("main{}", sourceExts.front());

	Output::getUserInput("Root source directory:", props.location, inputColor, [](std::string& input) {
		bool validChars = String::toLowerCase(input).find_first_not_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos;
		return validChars;
	});
	Output::getUserInput(fmt::format("Main source file:"), props.mainSource, inputColor, [&sourceExts](std::string& input) {
		auto lower = String::toLowerCase(input);
		bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz._") == std::string::npos;
		if (validChars && !String::endsWith(sourceExts, lower))
		{
			input = String::getPathBaseName(input) + sourceExts.front();
		}
		return validChars;
	});

	if (Output::getUserInputYesNo("Use a precompiled header?", inputColor))
	{
		std::string headerExt;
		if (props.language == CodeLanguage::C)
			headerExt = ".h";
		else
			headerExt = ".hpp";

		props.precompiledHeader = fmt::format("pch{}", headerExt);

		Output::getUserInput(fmt::format("Precompiled header:"), props.precompiledHeader, inputColor, [&headerExt](std::string& input) {
			auto lower = String::toLowerCase(input);
			bool validChars = lower.find_first_not_of("abcdefghijklmnopqrstuvwxyz._") == std::string::npos;
			if (validChars && !String::endsWith(headerExt, lower))
			{
				input = String::getPathBaseName(input) + headerExt;
				return true;
			}
			return validChars;
		});
	}

	Commands::sleep(0.5);

	Output::lineBreak();
	Output::print(Color::Black, separator);

	{
		Output::print(Color::Reset, fmt::format("{}/{}", props.location, props.mainSource));
		Output::lineBreak();

		auto mainCpp = StarterFileTemplates::getMainCxx(props.language);
		String::replaceAll(mainCpp, "\t", "   ");
		std::cout << Output::getAnsiStyle(Color::Blue) << mainCpp << Output::getAnsiReset() << std::endl;

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	if (!props.precompiledHeader.empty())
	{
		Output::print(Color::Reset, fmt::format("{}/{}", props.location, props.precompiledHeader));
		Output::lineBreak();

		const auto pch = StarterFileTemplates::getPch(props.precompiledHeader, props.language);
		std::cout << Output::getAnsiStyle(Color::Blue) << pch << Output::getAnsiReset() << std::endl;

		Output::lineBreak();
		Output::print(Color::Black, separator);
	}

	{
		Output::print(Color::Reset, "build.json");
		Output::lineBreak();

		auto jsonFile = StarterFileTemplates::getBuildJson(props);
		std::cout << Output::getAnsiStyle(Color::Blue) << jsonFile.dump(3, ' ') << Output::getAnsiReset() << std::endl;

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

		if (!makeGitIgnore())
			result = false;

		if (!makeDotEnv())
			result = false;
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

		Output::lineBreak();

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
	const auto contents = StarterFileTemplates::getMainCxx(inProps.language);

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makePch(const BuildJsonProps& inProps)
{
	const auto outFile = fmt::format("{}/{}/{}", m_rootPath, inProps.location, inProps.precompiledHeader);
	const auto contents = StarterFileTemplates::getPch(inProps.precompiledHeader, inProps.language);

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
