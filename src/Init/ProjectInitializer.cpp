/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Init/ProjectInitializer.hpp"

#include "Init/FileTemplates.hpp"
#include "Libraries/Format.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Diagnostic.hpp"
#include "Terminal/Output.hpp"
#include "Terminal/Path.hpp"
#include "Terminal/Unicode.hpp"
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
		Diagnostic::error(fmt::format("Path '{}' does not exit. Please create it first.", path));
		return false;
	}

	// At the moment, only initialize an empty path
	m_rootPath = Commands::getCanonicalPath(path);
	if (!Commands::pathIsEmpty(m_rootPath, { ".git", ".gitignore", "README.md", "LICENSE" }))
	{
		Diagnostic::error(fmt::format("Path '{}' is not empty. Please choose a different path, or clean this one first.", m_rootPath));
		return false;
	}

	Path::sanitize(m_rootPath);

	Output::print(Color::Yellow, fmt::format("Initializing a project called '{}'. This'll only take a moment...", m_inputs.initProjectName()));

	if (!makeBuildJson())
		return false;

	bool cleanOutput = true;
	Commands::makeDirectory(fmt::format("{}/src/", m_rootPath), cleanOutput);

	bool result = true;

	if (!makeMainCpp())
		result = false;

	if (!makePch())
		result = false;

	if (!makeGitIgnore())
		result = false;

	if (!makeDotEnv())
		result = false;

	if (result)
	{
		auto symbol = Unicode::heavyCheckmark();
		Output::displayStyledSymbol(Color::Green, symbol, "Done! Happy coding!");
	}
	else
	{
		auto symbol = Unicode::heavyBallotX();
		Output::displayStyledSymbol(Color::Red, symbol, "There was an error creating the project files.");
	}

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::makeBuildJson()
{
	const auto buildJsonPath = fmt::format("{}/build.json", m_rootPath);

	BuildJsonProps props;
	props.workspaceName = m_inputs.initProjectName();
	props.projectName = props.workspaceName;
	props.version = "0.0.1";
	props.language = CodeLanguage::CPlusPlus;

	auto jsonFile = FileTemplates::getBuildJson(props);

	JsonFile::saveToFile(jsonFile, buildJsonPath);

	return true;
}

/*****************************************************************************/
bool ProjectInitializer::makeMainCpp()
{
	const auto outFile = fmt::format("{}/src/main.cpp", m_rootPath);
	const auto contents = FileTemplates::getMainCpp();

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makePch()
{
	const auto outFile = fmt::format("{}/src/PCH.hpp", m_rootPath);
	const auto contents = FileTemplates::getPch();

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeGitIgnore()
{
	const auto outFile = fmt::format("{}/.gitignore", m_rootPath);
	const auto contents = FileTemplates::getGitIgnore();

	return Commands::createFileWithContents(outFile, contents);
}

/*****************************************************************************/
bool ProjectInitializer::makeDotEnv()
{
	const auto outFile = fmt::format("{}/.env", m_rootPath);
	const auto contents = FileTemplates::getDotEnv();

	return Commands::createFileWithContents(outFile, contents);
}
}
