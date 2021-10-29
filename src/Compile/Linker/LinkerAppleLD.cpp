/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerAppleLD.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerAppleLD::LinkerAppleLD(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMLLD(inState, inProject)
{
}

// ="-Wl,-flat_namespace,-undefined,suppress

/*****************************************************************************/
StringList LinkerAppleLD::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));

	ret.emplace_back("-dynamiclib");
	// ret.emplace_back("-fPIC");
	// ret.emplace_back("-flat_namespace");

	UNUSED(outputFileBase);

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret, std::string());

	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addMacosFrameworkOptions(ret);

	addLibDirs(ret);

	ret.emplace_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
void LinkerAppleLD::addStripSymbolsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleLD::addThreadModelLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleLD::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleLD::addLibStdCppLinkerOption(StringList& outArgList) const
{
	if (m_project.language() == CodeLanguage::CPlusPlus)
	{
		std::string flag{ "-stdlib=libc++" };
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));

		// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
	}
}

/*****************************************************************************/
bool LinkerAppleLD::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() != Arch::Cpu::UniversalMacOS)
#endif
	{
		if (!LinkerLLVMLLD::addArchitecture(outArgList, inArch))
			return false;

		if (!CompilerCxxAppleClang::addArchitectureToCommand(outArgList, m_state))
			return false;
	}
#if defined(CHALET_MACOS)
	else
	{
		if (!CompilerCxxAppleClang::addMultiArchOptionsToCommand(outArgList, inArch, m_state))
			return false;
	}
#endif

	return true;
}

/*****************************************************************************/
void LinkerAppleLD::addObjectiveCxxLink(StringList& outArgList) const
{
	// Unused in AppleClang
	UNUSED(outArgList);
}

}
