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
	// StringList getMsvcObjectFileList(const StringList& inObjects);

	std::string getCompileEchoAsm(const std::string& file);
	std::string getCompileEchoSources(const std::string& file);
	std::string getCompileEchoLinker(const std::string& file);

	std::string getBuildRecipes(const SourceOutputs& inOutputs);

	std::string getPchBuildRecipe(const std::string& pchTarget);
	std::string getObjBuildRecipes(const StringList& inObjects, const std::string& pchTarget);

	std::string getRcRecipe(const std::string& source, const std::string& object, const std::string& pchTarget);
	std::string getCppRecipe(const std::string& source, const std::string& object, const std::string& pchTarget);

	std::string getQuietFlag();
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
