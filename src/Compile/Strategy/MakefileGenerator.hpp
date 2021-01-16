/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_GENERATOR_HPP
#define CHALET_MAKEFILE_GENERATOR_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct MakefileGenerator
{
	explicit MakefileGenerator(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain);

	std::string getContents(const SourceOutputs& inOutputs);

private:
	std::string getUnicodeLinkingCommand();
	std::string getColorCommand(const ushort inId);

	//
	std::string getCompileFlags();

	std::string getBlueColor();
	std::string getMagentaColor();
	std::string getQuietFlag();
	std::string getMoveCommand();
	std::string getCompileEchoAsm();
	std::string getCompileEchoSources();
	std::string getCompileEchoLinker();

	std::string getDumpAsmRecipe();
	std::string getAsmRecipe();
	std::string getMakePchRecipe();
	std::string getPchRecipe();
	std::string getRcRecipe();
	std::string getCppRecipe(const std::string& ext);
	std::string getObjcRecipe(const std::string& ext);
	std::string getTargetRecipe();

	const BuildState& m_state;
	const ProjectConfiguration& m_project;
	CompileToolchain& m_toolchain;

	bool m_cleanOutput = false;
};
}

#endif // CHALET_MAKEFILE_GENERATOR_HPP
