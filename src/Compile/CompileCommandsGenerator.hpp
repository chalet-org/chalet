/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CompileToolchainController.hpp"

namespace chalet
{
class BuildState;
struct SourceTarget;
struct SourceOutputs;
struct SourceFileGroup;

struct CompileCommandsGenerator
{
	explicit CompileCommandsGenerator(const BuildState& inState);
	CHALET_DISALLOW_COPY_MOVE(CompileCommandsGenerator);
	~CompileCommandsGenerator();

	bool addCompileCommands(CompileToolchain& inToolchain, const SourceOutputs& inOutputs);
	bool addCompileCommandsStubsFromState();

	bool save() const;
	bool saveStub(const std::string& inPath) const;

	bool fileExists() const;

	void addCompileCommand(const std::string& inFile, StringList&& inCommand);

private:
	const std::string& getSourceFile(const SourceFileGroup& inGroup) const;
	StringList getCommand(CompileToolchain& inToolchain, const SourceFileGroup& inGroup) const;

	const BuildState& m_state;

	struct CompileCommand;
	std::vector<Unique<CompileCommand>> m_compileCommands;

	const std::string kCompileCommandsJson;
};
}
