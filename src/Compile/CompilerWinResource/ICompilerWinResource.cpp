/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/ICompilerWinResource.hpp"

#include "Compile/CommandAdapter/CommandAdapterWinResource.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/CompilerWinResource/CompilerWinResourceGNUWindRes.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceLLVMRC.hpp"
#include "Compile/CompilerWinResource/CompilerWinResourceVisualStudioRC.hpp"

namespace chalet
{
/*****************************************************************************/
ICompilerWinResource::ICompilerWinResource(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<ICompilerWinResource> ICompilerWinResource::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	UNUSED(inType);

	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	// LOG("ICompilerWinResource:", static_cast<i32>(inType), executable);

	if (String::equals("rc", executable))
		return std::make_unique<CompilerWinResourceVisualStudioRC>(inState, inProject);
	else if (String::equals("llvm-rc", executable))
		return std::make_unique<CompilerWinResourceLLVMRC>(inState, inProject);

	return std::make_unique<CompilerWinResourceGNUWindRes>(inState, inProject);
}

/*****************************************************************************/
bool ICompilerWinResource::initialize()
{
	CommandAdapterWinResource adapter(m_state, m_project);
	if (!adapter.createWindowsApplicationManifest())
		return false;

	if (!adapter.createWindowsApplicationIcon())
		return false;

	return true;
}

/*****************************************************************************/
void ICompilerWinResource::addIncludes(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ICompilerWinResource::addDefines(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
