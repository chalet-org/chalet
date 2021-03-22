/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_MAKEFILE_GENERATOR_NMAKE_HPP
#define CHALET_MAKEFILE_GENERATOR_NMAKE_HPP

#include "Compile/Toolchain/ICompileToolchain.hpp"
#include "State/BuildState.hpp"
#include "State/SourceOutputs.hpp"

namespace chalet
{
struct MakefileGeneratorNMake
{
#if defined(CHALET_WIN32)
	explicit MakefileGeneratorNMake(const BuildState& inState, const ProjectConfiguration& inProject, CompileToolchain& inToolchain);

	std::string getContents(const SourceOutputs& inOutputs);

private:
	//
	std::string getCompileEchoAsm();
	std::string getCompileEchoSources();
	std::string getCompileEchoLinker();

	std::string getDumpAsmRecipe();
	std::string getAsmRecipe();
	std::string getMakePchRecipe();
	std::string getPchRecipe();
	std::string getRcRecipe(const std::string& ext);
	std::string getCppRecipe(const std::string& ext);
	std::string getTargetRecipe();

	std::string getPchOrderOnlyPreReq();

	std::string getQuietFlag();
	std::string getMoveCommand(std::string inInput, std::string inOutput);
	std::string getPrinter(const std::string& inPrint = "");

	std::string getColorBlue();
	std::string getColorPurple();

	const BuildState& m_state;
	const ProjectConfiguration& m_project;
	CompileToolchain& m_toolchain;

	bool m_cleanOutput = false;
	bool m_generateDependencies = false;
#else
	MakefileGeneratorNMake();
#endif
};
}

#endif // CHALET_MAKEFILE_GENERATOR_NMAKE_HPP
