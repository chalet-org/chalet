/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerLLVMClang.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "Compile/Linker/LinkerVisualStudioLINK.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerLLVMClang::LinkerLLVMClang(const BuildState& inState, const SourceTarget& inProject) :
	LinkerGCC(inState, inProject),
	m_clangAdapter(inState, inProject)
{
}

/*****************************************************************************/
void LinkerLLVMClang::addLinks(StringList& outArgList) const
{
	LinkerGCC::addLinks(outArgList);

	if (m_state.environment->isWindowsTarget())
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
void LinkerLLVMClang::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerLLVMClang::addProfileInformation(StringList& outArgList) const
{
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
void LinkerLLVMClang::addFuseLdOption(StringList& outArgList) const
{
	UNUSED(outArgList);
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
	if (!m_state.environment->isWindowsTarget())
	{
		if (m_project.positionIndependentCode())
		{
			std::string option{ "-fPIC" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
		else if (m_project.positionIndependentExecutable())
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
