/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_COMPILE_COMMANDS_GENERATOR_HPP
#define CHALET_COMPILE_COMMANDS_GENERATOR_HPP

#include "Compile/CompileToolchainController.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;
struct SourceOutputs;

struct CompileCommandsGenerator
{
	explicit CompileCommandsGenerator(const BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileCommandsGenerator);
	~CompileCommandsGenerator();

	bool addCompileCommands(CompileToolchain& inToolchain, const SourceOutputs& inOutputs);

	bool save() const;

private:
	void addCompileCommand(const std::string& inFile, StringList&& inCommand);
	void addCompileCommand(const std::string& inFile, std::string&& inCommand);

	const BuildState& m_state;

	struct CompileCommand;
	std::vector<Unique<CompileCommand>> m_compileCommands;
};
}

#endif // CHALET_COMPILE_COMMANDS_GENERATOR_HPP
