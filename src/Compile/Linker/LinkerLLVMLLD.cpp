/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerLLVMLLD.hpp"

#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/Linker/LinkerVisualStudioLINK.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerLLVMLLD::LinkerLLVMLLD(const BuildState& inState, const SourceTarget& inProject) :
	LinkerGNULD(inState, inProject)
{
}

/*****************************************************************************/
StringList LinkerLLVMLLD::getLinkExclusions() const
{
	return { "stdc++fs" };
}

/*****************************************************************************/
void LinkerLLVMLLD::addLinks(StringList& outArgList) const
{
	LinkerGNULD::addLinks(outArgList);

	if (m_state.environment->isWindowsClang())
	{
		const std::string prefix{ "-l" };
		for (const char* link : {
				 "DbgHelp",
				 "kernel32",
				 "user32",
				 "gdi32",
				 "winspool",
				 "shell32",
				 "ole32",
				 "oleaut32",
				 "uuid",
				 "comdlg32",
				 "advapi32",
			 })
		{
			List::addIfDoesNotExist(outArgList, fmt::format("{}{}", prefix, link));
		}
	}
}

/*****************************************************************************/
void LinkerLLVMLLD::addStripSymbols(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMLLD::addLinkerScripts(StringList& outArgList) const
{
	// TODO: Check if there's a clang/apple clang version of this
	// If using LD with "-fuse-ld=lld-link", you can pass linkerscripts supposedly
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMLLD::addLibStdCppLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMLLD::addStaticCompilerLibraries(StringList& outArgList) const
{
	if (m_project.staticLinking())
	{
		std::string flag{ "-static-libsan" };
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));

		// TODO: Investigate for other -static candidates on clang/mac
	}
}

/*****************************************************************************/
void LinkerLLVMLLD::addSubSystem(StringList& outArgList) const
{
	if (m_state.environment->isWindowsClang())
	{
		const ProjectKind kind = m_project.kind();
		if (kind == ProjectKind::Executable)
		{
			const auto subSystem = LinkerVisualStudioLINK::getMsvcCompatibleSubSystem(m_project);
			List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/subsystem:{}", subSystem));
		}
	}
}

/*****************************************************************************/
void LinkerLLVMLLD::addEntryPoint(StringList& outArgList) const
{
	if (m_state.environment->isWindowsClang())
	{
		const auto entryPoint = LinkerVisualStudioLINK::getMsvcCompatibleEntryPoint(m_project);
		if (!entryPoint.empty())
		{
			List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/entry:{}", entryPoint));
		}
	}
}

/*****************************************************************************/
bool LinkerLLVMLLD::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxClang::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
// TOOD: I think Clang on Linux could still use the system linker (LD), in which case,
//   These flags could be used
void LinkerLLVMLLD::startStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void LinkerLLVMLLD::endStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void LinkerLLVMLLD::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
