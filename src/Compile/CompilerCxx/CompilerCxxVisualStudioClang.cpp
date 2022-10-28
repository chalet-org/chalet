/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxVisualStudioClang.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxVisualStudioClang::CompilerCxxVisualStudioClang(const BuildState& inState, const SourceTarget& inProject) :
	CompilerCxxClang(inState, inProject),
	m_msvcAdapter(inState, inProject)
{
}

/*****************************************************************************/
void CompilerCxxVisualStudioClang::addDefines(StringList& outArgList) const
{
	StringList defines;

	auto crtType = m_msvcAdapter.getRuntimeLibraryType();

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

	CompilerCxxClang::addDefines(outArgList);
}
}
