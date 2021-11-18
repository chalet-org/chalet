/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileCommandsGenerator.hpp"

#include "State/SourceOutputs.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Path.hpp"
#include "Utility/String.hpp"
#include "Json/JsonFile.hpp"

namespace chalet
{
struct CompileCommandsGenerator::CompileCommand
{
	std::string file;
	std::string command;
};

/*****************************************************************************/
CompileCommandsGenerator::CompileCommandsGenerator() = default;

/*****************************************************************************/
CompileCommandsGenerator::~CompileCommandsGenerator() = default;

/*****************************************************************************/
bool CompileCommandsGenerator::addCompileCommands(CompileToolchain& inToolchain, SourceOutputs& inOutputs)
{
	auto getCommand = [&inToolchain](const SourceFileGroup& group) -> StringList {
		const auto& source = group.sourceFile;
		const auto& object = group.objectFile;
		bool generateDeps = false;
		std::string dep;
		// Note: No windows resource files
		switch (group.type)
		{
			case SourceType::CxxPrecompiledHeader:
				return inToolchain->compilerCxx->getPrecompiledHeaderCommand(source, object, generateDeps, dep, std::string());

			case SourceType::C:
				return inToolchain->compilerCxx->getCommand(source, object, generateDeps, dep, CxxSpecialization::C);

			case SourceType::CPlusPlus:
				return inToolchain->compilerCxx->getCommand(source, object, generateDeps, dep, CxxSpecialization::CPlusPlus);

			case SourceType::ObjectiveC:
				return inToolchain->compilerCxx->getCommand(source, object, generateDeps, dep, CxxSpecialization::ObjectiveC);

			case SourceType::ObjectiveCPlusPlus:
				return inToolchain->compilerCxx->getCommand(source, object, generateDeps, dep, CxxSpecialization::ObjectiveCPlusPlus);

			case SourceType::WindowsResource:
			case SourceType::Unknown:
			default:
				break;
		}

		return StringList();
	};

	for (auto& group : inOutputs.groups)
	{
		StringList outCommand = getCommand(*group);
		if (outCommand.empty())
			continue;

		addCompileCommand(group->sourceFile, std::move(outCommand));
	}

	return true;
}

/*****************************************************************************/
void CompileCommandsGenerator::addCompileCommand(const std::string& inFile, StringList&& inCommand)
{
	auto command = String::join(std::move(inCommand));
	addCompileCommand(inFile, std::move(command));
}

/*****************************************************************************/
void CompileCommandsGenerator::addCompileCommand(const std::string& inFile, std::string&& inCommand)
{
	auto compileCommand = std::make_unique<CompileCommand>();
	compileCommand->file = inFile;
	compileCommand->command = std::move(inCommand);
	m_compileCommands.push_back(std::move(compileCommand));
}

/*****************************************************************************/
bool CompileCommandsGenerator::save(const std::string& inOutputFolder) const
{
	auto outputFile = fmt::format("{}/compile_commands.json", inOutputFolder);
	auto cwd = Commands::getWorkingDirectory();
	Path::sanitize(cwd);

	Json outJson = Json::array();

	for (auto& command : m_compileCommands)
	{
		Json node;
		node = Json::object();
		node["directory"] = cwd;
		node["command"] = command->command;
		node["file"] = fmt::format("{}/{}", cwd, command->file);

		outJson.push_back(std::move(node));
	}

	if (!JsonFile::saveToFile(outJson, outputFile))
	{
		Diagnostic::error("compile_commands.json could not be saved.");
	}

	return true;
}

}
