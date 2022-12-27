/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerLLVMClang.hpp"

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "Compile/Linker/LinkerVisualStudioLINK.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerLLVMClang::LinkerLLVMClang(const BuildState& inState, const SourceTarget& inProject) :
	LinkerGCC(inState, inProject)
{
}

/*****************************************************************************/
void LinkerLLVMClang::addLinks(StringList& outArgList) const
{
	LinkerGCC::addLinks(outArgList);

	if (m_state.environment->isWindowsClang() || m_state.environment->isMingwClang() || m_state.environment->isWindowsTarget())
	{
		const std::string prefix{ "-l" };
		auto win32Links = getWin32CoreLibraryLinks();
		for (const auto& link : win32Links)
		{
			List::addIfDoesNotExist(outArgList, fmt::format("{}{}", prefix, link));
		}
	}
}

/*****************************************************************************/
void LinkerLLVMClang::addStripSymbols(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMClang::addLinkerScripts(StringList& outArgList) const
{
	// TODO: Check if there's a clang/apple clang version of this
	// If using LD with "-fuse-ld=lld-link", you can pass linkerscripts supposedly
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMClang::addProfileInformation(StringList& outArgList) const
{
	// TODO: isExecutable or !isSharedLibrary
	if (m_state.configuration.enableProfiling() && m_project.isExecutable())
	{
		std::string profileInfo{ "-pg" };
		// if (isFlagSupported(profileInfo))
		outArgList.emplace_back(std::move(profileInfo));
	}
}

/*****************************************************************************/
void LinkerLLVMClang::addLibStdCppLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMClang::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxClang::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
void LinkerLLVMClang::addStaticCompilerLibraries(StringList& outArgList) const
{
	if (m_project.staticRuntimeLibrary())
	{
		// TODO: Investigate for other -static candidates on clang/mac

		if (m_state.configuration.sanitizeAddress())
		{
			std::string flag{ "-static-libsan" };
			// if (isFlagSupported(flag))
			List::addIfDoesNotExist(outArgList, std::move(flag));
		}
	}
}

/*****************************************************************************/
bool LinkerLLVMClang::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxClang::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
void LinkerLLVMClang::addCppFilesystem(StringList& outArgList) const
{
	if (m_project.cppFilesystem())
	{
		if (m_versionMajorMinor >= 700 && m_versionMajorMinor < 900)
		{
			std::string option{ "-lc++-fs" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}

		if (m_versionMajorMinor < 700)
		{
			std::string option{ "-lc++-experimental" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
	}
}

/*****************************************************************************/
void LinkerLLVMClang::addPositionIndependentCodeOption(StringList& outArgList) const
{
	if (!m_state.environment->isMingw() && !m_state.environment->isWindowsTarget())
	{
		if (m_project.platformIndependentCode())
		{
			std::string option{ "-fPIC" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
		else if (m_project.platformIndependentExecutable())
		{
			std::string option{ "-fPIE" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
	}
}

/*****************************************************************************/
void LinkerLLVMClang::startStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void LinkerLLVMClang::endStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void LinkerLLVMClang::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}
}
