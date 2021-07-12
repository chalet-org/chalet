/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_COMMANDS_GENERATOR_HPP
#define CHALET_COMPILE_COMMANDS_GENERATOR_HPP

namespace chalet
{
struct CompileCommandsGenerator
{
	CompileCommandsGenerator();
	~CompileCommandsGenerator();

	void addCompileCommand(const std::string& inFile, const StringList& inCommand);
	void addCompileCommand(const std::string& inFile, const std::string& inCommand);

private:
	struct CompileCommand;
	std::vector<std::unique_ptr<CompileCommand>> m_compileCommands;
};
}

#endif // CHALET_COMPILE_COMMANDS_GENERATOR_HPP
