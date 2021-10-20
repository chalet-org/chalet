/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_COMMANDS_GENERATOR_HPP
#define CHALET_COMPILE_COMMANDS_GENERATOR_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"

namespace chalet
{
struct SourceTarget;
struct SourceOutputs;

struct CompileCommandsGenerator
{
	CompileCommandsGenerator();
	CHALET_DISALLOW_COPY_MOVE(CompileCommandsGenerator);
	~CompileCommandsGenerator();

	bool addCompileCommands(CompileToolchain& inToolchain, SourceOutputs& inOutputs);

	bool save(const std::string& inOutputFolder) const;

private:
	void addCompileCommand(const std::string& inFile, StringList&& inCommand);
	void addCompileCommand(const std::string& inFile, std::string&& inCommand);

	struct CompileCommand;
	std::vector<Unique<CompileCommand>> m_compileCommands;
};
}

#endif // CHALET_COMPILE_COMMANDS_GENERATOR_HPP
