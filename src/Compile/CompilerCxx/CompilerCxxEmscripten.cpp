/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxEmscripten.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxEmscripten::CompilerCxxEmscripten(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxClang(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxEmscripten::initialize()
{
	if (!CompilerCxxClang::initialize())
		return false;

	return true;
}

/*****************************************************************************/
bool CompilerCxxEmscripten::addExecutable(StringList& outArgList) const
{
	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;
	if (executable.empty())
		return false;

	auto& python = m_state.environment->commandInvoker();

	outArgList.emplace_back(getQuotedPath(python));
	outArgList.emplace_back(getQuotedPath(executable));

	return true;
}

/*****************************************************************************/
void CompilerCxxEmscripten::addPositionIndependentCodeOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-fPIC");
}
}
