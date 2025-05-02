/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileCommandsGenerator.hpp"

#include "Core/CommandLineInputs.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/SourceOutputs.hpp"
#include "State/Target/CMakeTarget.hpp"
#include "State/Target/MesonTarget.hpp"
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
	StringList arguments;
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

	inToolchain->setQuotedPaths(false);
	inToolchain->setGenerateDependencies(false);

	std::string dummyArch;
	for (auto& group : inOutputs.groups)
	{
#if defined(CHALET_MACOS)
		if (group->type == SourceType::CxxPrecompiledHeader && m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
		{
			for (auto& arch : m_state.inputs.universalArches())
			{
				addCompileCommand(getSourceFile(*group, arch), getCommand(inToolchain, *group, arch));
			}
		}
		else
#endif
		{
			addCompileCommand(getSourceFile(*group, dummyArch), getCommand(inToolchain, *group, dummyArch));
		}
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
				addCompileCommand(file, StringList{});

			if (project.usesPrecompiledHeader())
				addCompileCommand(project.precompiledHeader(), StringList{});
		}
	}

	return true;
}

/*****************************************************************************/
std::string CompileCommandsGenerator::getSourceFile(const SourceFileGroup& inGroup, const std::string& inArch) const
{
	if (inGroup.type == SourceType::CxxPrecompiledHeader)
	{
		if (!inGroup.otherFile.empty())
			return inGroup.otherFile;
		else
		{
#if !defined(CHALET_MACOS)
			UNUSED(inArch);
#endif
#if defined(CHALET_MACOS)
			if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
			{
				auto baseFolder = String::getPathFolder(inGroup.objectFile);
				auto filename = String::getPathFilename(inGroup.objectFile);
				return fmt::format("{}_{}/{}", baseFolder, inArch, filename);
			}
			else
#endif
			{
				return inGroup.objectFile;
			}
		}
	}
	else
	{
		return inGroup.sourceFile;
	}
}

/*****************************************************************************/
StringList CompileCommandsGenerator::getCommand(CompileToolchain& inToolchain, const SourceFileGroup& inGroup, const std::string& inArch) const
{
	const auto& source = inGroup.sourceFile;
	const auto& object = inGroup.objectFile;
	std::string dep;
	// Note: No windows resource files

	switch (inGroup.type)
	{
		case SourceType::CxxPrecompiledHeader: {
#if defined(CHALET_MACOS)
			if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
			{
				auto baseFolder = String::getPathFolder(object);
				auto filename = String::getPathFilename(object);
				auto outObject = fmt::format("{}_{}/{}", baseFolder, inArch, filename);
				return inToolchain->compilerCxx->getPrecompiledHeaderCommand(source, outObject, dep, inArch);
			}
			else
#endif
			{
				return inToolchain->compilerCxx->getPrecompiledHeaderCommand(source, object, dep, inArch);
			}
		}

		case SourceType::C:
		case SourceType::CPlusPlus:
		case SourceType::ObjectiveC:
		case SourceType::ObjectiveCPlusPlus:
			return inToolchain->compilerCxx->getCommand(source, object, dep, inGroup.type);

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

	auto compileCommand = std::make_unique<CompileCommand>();
	compileCommand->file = inFile;
	compileCommand->arguments = std::move(inCommand);
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
		node["arguments"] = command->arguments;
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
		std::string targetFolder;
		for (auto& target : m_state.targets)
		{
			if (target->isCMake())
			{
				auto& project = static_cast<const CMakeTarget&>(*target);
				targetFolder = project.targetFolder();
			}
			else if (target->isMeson())
			{
				auto& project = static_cast<const MesonTarget&>(*target);
				targetFolder = project.targetFolder();
			}
		}
		if (!targetFolder.empty())
		{
			auto lastCompileCommands = fmt::format("{}/{}/{}", buildOutputDir, targetFolder, kCompileCommandsJson);
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
		node["arguments"] = command->arguments;
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
