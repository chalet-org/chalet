/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileCommandsGenerator.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "System/Files.hpp"
#include "Utility/Path.hpp"
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
CompileCommandsGenerator::CompileCommandsGenerator(const BuildState& inState) :
	m_state(inState)
{
}

/*****************************************************************************/
CompileCommandsGenerator::~CompileCommandsGenerator() = default;

/*****************************************************************************/
bool CompileCommandsGenerator::addCompileCommands(CompileToolchain& inToolchain, const SourceOutputs& inOutputs)
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
			case SourceType::CPlusPlus:
			case SourceType::ObjectiveC:
			case SourceType::ObjectiveCPlusPlus:
				return inToolchain->compilerCxx->getCommand(source, object, generateDeps, dep, group.type);

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
bool CompileCommandsGenerator::save() const
{
	const auto& outputDirectory = m_state.paths.outputDirectory();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto outputFile = fmt::format("{}/compile_commands.json", buildOutputDir);

	Json outJson = Json::array();

	for (auto& command : m_compileCommands)
	{
		Json node;
		node = Json::object();
		node["directory"] = m_state.inputs.workingDirectory();
		node["command"] = command->command;
		node["file"] = Files::getCanonicalPath(command->file);

		outJson.push_back(std::move(node));
	}

	if (!m_compileCommands.empty())
	{
		if (!JsonFile::saveToFile(outJson, outputFile))
		{
			Diagnostic::error("compile_commands.json could not be saved.");
			return false;
		}
		if (!Files::copySilent(outputFile, outputDirectory))
		{
			Diagnostic::error("compile_commands.json could not be copied to: '{}'", outputDirectory);
			return false;
		}
	}
	else
	{
		const CMakeTarget* lastTarget = nullptr;
		for (auto& target : m_state.targets)
		{
			if (target->isCMake())
			{
				lastTarget = static_cast<const CMakeTarget*>(target.get());
			}
		}
		if (lastTarget != nullptr)
		{
			auto lastCompileCommands = fmt::format("{}/{}/compile_commands.json", buildOutputDir, lastTarget->targetFolder());
			if (Files::pathExists(lastCompileCommands))
			{
				if (!Files::copySilent(lastCompileCommands, outputDirectory))
				{
					Diagnostic::error("compile_commands.json could not be copied to: '{}'", outputDirectory);
					return false;
				}
			}
		}
	}

	return true;
}

}
