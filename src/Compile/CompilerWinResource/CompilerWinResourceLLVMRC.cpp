/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/CompilerWinResourceLLVMRC.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerWinResourceLLVMRC::CompilerWinResourceLLVMRC(const BuildState& inState, const SourceTarget& inProject) :
	CompilerWinResourceGNUWindRes(inState, inProject)
{
}

/*****************************************************************************/
StringList CompilerWinResourceLLVMRC::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency)
{
	UNUSED(generateDependency, dependency);

	StringList ret;

	if (!m_state.toolchain.canCompilerWindowsResources())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.compilerWindowsResource()));

	// llvm-rc is basically rc.exe w/ GNU-style args
	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back("-Fo");
	ret.push_back(outputFile);
	ret.push_back(inputFile);

	return ret;
}
}
