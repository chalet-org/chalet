/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompileCommandsGenerator.hpp"

#include "Utility/String.hpp"

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
void CompileCommandsGenerator::addCompileCommand(const std::string& inFile, const StringList& inCommand)
{
	auto command = String::join(inCommand);
	addCompileCommand(inFile, command);
}

/*****************************************************************************/
void CompileCommandsGenerator::addCompileCommand(const std::string& inFile, const std::string& inCommand)
{
	auto compileCommand = std::make_unique<CompileCommand>();
	compileCommand->file = inFile;
	compileCommand->command = inCommand;
	m_compileCommands.push_back(std::move(compileCommand));
}

}
