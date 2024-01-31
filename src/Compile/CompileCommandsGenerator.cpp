/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileCommandsGenerator.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/SourceTarget.hpp"
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
	m_state(inState),
	kCompileCommandsJson("compile_commands.json")
{
}

/*****************************************************************************/
CompileCommandsGenerator::~CompileCommandsGenerator() = default;

/*****************************************************************************/
bool CompileCommandsGenerator::addCompileCommands(CompileToolchain& inToolchain, const SourceOutputs& inOutputs)
{
	bool quotedPaths = inToolchain->linker->quotedPaths();
	bool generateDependencies = inToolchain->linker->generateDependencies();

	inToolchain->setQuotedPaths(true);
	inToolchain->setGenerateDependencies(false);

	for (auto& group : inOutputs.groups)
	{
		addCompileCommand(getSourceFile(*group), getCommand(inToolchain, *group));
	}

	inToolchain->setQuotedPaths(quotedPaths);
	inToolchain->setGenerateDependencies(generateDependencies);

	return true;
}

/*****************************************************************************/
bool CompileCommandsGenerator::addCompileCommandsStubsFromState()
{
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			const auto& project = static_cast<const SourceTarget&>(*target);
			const auto& files = project.files();
			for (auto& file : files)
				addCompileCommand(file, std::string());

			if (project.usesPrecompiledHeader())
				addCompileCommand(project.precompiledHeader(), std::string());
		}
	}

	return true;
}

const std::string& CompileCommandsGenerator::getSourceFile(const SourceFileGroup& inGroup) const
{
	if (inGroup.type == SourceType::CxxPrecompiledHeader)
	{
		if (!inGroup.otherFile.empty())
			return inGroup.otherFile;
		else
			return inGroup.objectFile;
	}
	else
	{
		return inGroup.sourceFile;
	}
}

/*****************************************************************************/
StringList CompileCommandsGenerator::getCommand(CompileToolchain& inToolchain, const SourceFileGroup& inGroup) const
{
	const auto& source = inGroup.sourceFile;
	const auto& object = inGroup.objectFile;
	std::string dep;
	// Note: No windows resource files

	switch (inGroup.type)
	{
		case SourceType::CxxPrecompiledHeader:
			return inToolchain->compilerCxx->getPrecompiledHeaderCommand(source, object, dep, std::string());

		case SourceType::C:
		case SourceType::CPlusPlus:
		case SourceType::ObjectiveC:
		case SourceType::ObjectiveCPlusPlus: {
			return inToolchain->compilerCxx->getCommand(source, object, dep, inGroup.type);
		}

		case SourceType::WindowsResource:
		case SourceType::Unknown:
		default:
			break;
	}

	return StringList();
}

/*****************************************************************************/
void CompileCommandsGenerator::addCompileCommand(const std::string& inFile, StringList&& inCommand)
{
	if (inCommand.empty())
		return;

	addCompileCommand(inFile, String::join(std::move(inCommand)));
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
	auto outputFile = fmt::format("{}/{}", buildOutputDir, kCompileCommandsJson);

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
			Diagnostic::error("There was a problem saving: {}", outputFile);
			return false;
		}

		if (!String::equals(String::getPathFolder(outputFile), outputDirectory))
		{
			if (!Files::copySilent(outputFile, outputDirectory))
			{
				Diagnostic::error("{} could not be copied to: '{}'", kCompileCommandsJson, outputDirectory);
				return false;
			}
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
			auto lastCompileCommands = fmt::format("{}/{}/{}", buildOutputDir, lastTarget->targetFolder(), kCompileCommandsJson);
			if (Files::pathExists(lastCompileCommands))
			{
				if (!Files::copySilent(lastCompileCommands, outputDirectory))
				{
					Diagnostic::error("{} could not be copied to: '{}'", kCompileCommandsJson, outputDirectory);
					return false;
				}
			}
		}
	}

	return true;
}

/*****************************************************************************/
bool CompileCommandsGenerator::saveStub(const std::string& outputFile) const
{
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

	if (!JsonFile::saveToFile(outJson, outputFile))
	{
		Diagnostic::error("There was a problem saving: {}", outputFile);
		return false;
	}

	return true;
}

/*****************************************************************************/
bool CompileCommandsGenerator::fileExists() const
{
	const auto& outputDirectory = m_state.paths.outputDirectory();
	const auto& buildOutputDir = m_state.paths.buildOutputDir();
	auto outputCC = fmt::format("{}/{}", outputDirectory, kCompileCommandsJson);
	auto buildCC = fmt::format("{}/{}", buildOutputDir, kCompileCommandsJson);

	return Files::pathExists(outputCC) && Files::pathExists(buildCC);
}

}
