/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerWinResource/CompilerWinResourceLLVMRC.hpp"

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

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

	if (!m_state.toolchain.canCompileWindowsResources())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.compilerWindowsResource()));

	// llvm-rc is basically rc.exe w/ GNU-style args
	addDefines(ret);
	addIncludes(ret);

	ret.emplace_back("-Fo");
	ret.emplace_back(getQuotedPath(outputFile));
	ret.emplace_back(getQuotedPath(inputFile));

	return ret;
}

/*****************************************************************************/
void CompilerWinResourceLLVMRC::addDefines(StringList& outArgList) const
{
#if defined(CHALET_WIN32)
	if (m_state.environment->isWindowsClang())
	{

		StringList defines;

		CommandAdapterMSVC msvcAdapter(m_state, m_project);
		auto crtType = msvcAdapter.getRuntimeLibraryType();

		// https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library?view=msvc-170

		defines.emplace_back("_MT");

		if (crtType == WindowsRuntimeLibraryType::MultiThreadedDLL
			|| crtType == WindowsRuntimeLibraryType::MultiThreadedDebugDLL)
		{
			defines.emplace_back("_DLL");
		}

		if (crtType == WindowsRuntimeLibraryType::MultiThreadedDebugDLL
			|| crtType == WindowsRuntimeLibraryType::MultiThreadedDebug)
		{
			defines.emplace_back("_DEBUG");
		}

		const std::string prefix{ "-D" };
		for (auto& define : defines)
		{
			List::addIfDoesNotExist(outArgList, prefix + define);
		}
	}
#endif

	CompilerWinResourceGNUWindRes::addDefines(outArgList);
}
}
