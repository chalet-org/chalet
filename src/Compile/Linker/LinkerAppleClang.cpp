/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerAppleClang.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxClang.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerAppleClang::LinkerAppleClang(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMClang(inState, inProject)
{
}

// ="-Wl,-flat_namespace,-undefined,suppress

/*****************************************************************************/
StringList LinkerAppleClang::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	StringList ret;

	m_outputFileBase = outputFileBase;

	if (!addExecutable(ret))
		return ret;

	ret.emplace_back("-dynamiclib");
	// ret.emplace_back("-flat_namespace");

	addStripSymbols(ret);
	addLinkerOptions(ret);
	addSystemRootOption(ret);
	addProfileInformation(ret);
	addLinkTimeOptimizations(ret);
	addThreadModelLinks(ret);
	addArchitecture(ret, std::string());

	addLibStdCppLinkerOption(ret);
	addSanitizerOptions(ret);
	addStaticCompilerLibraries(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addAppleFrameworkOptions(ret);
	addRunPath(ret);

	addLibDirs(ret);
	addSystemLibDirs(ret);

	ret.emplace_back("-o");
	ret.emplace_back(getQuotedPath(outputFile));
	addSourceObjects(ret, sourceObjs);

	addCppFilesystem(ret);
	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
void LinkerAppleClang::addStripSymbols(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleClang::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleClang::addProfileInformation(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerAppleClang::addLibStdCppLinkerOption(StringList& outArgList) const
{
	if (m_project.language() == CodeLanguage::CPlusPlus)
	{
		auto flag = fmt::format("-stdlib={}", m_clangAdapter.getCxxLibrary());
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));

		// Note: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
		//  but it is not supported in later versions of Xcode
	}
}

/*****************************************************************************/
void LinkerAppleClang::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxAppleClang::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
bool LinkerAppleClang::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
#if defined(CHALET_MACOS)
	if (m_state.info.targetArchitecture() == Arch::Cpu::UniversalMacOS)
	{
		if (!CompilerCxxAppleClang::addMultiArchOptionsToCommand(outArgList, inArch, m_state))
			return false;

		if (!CompilerCxxAppleClang::addOsTargetOptions(outArgList, m_state, m_versionMajorMinor))
			return false;
	}
	else
#endif
	{
		if (!LinkerLLVMClang::addArchitecture(outArgList, inArch))
			return false;

		if (!CompilerCxxAppleClang::addOsTargetOptions(outArgList, m_state, m_versionMajorMinor))
			return false;
	}

	return true;
}

/*****************************************************************************/
bool LinkerAppleClang::addSystemRootOption(StringList& outArgList) const
{
	return CompilerCxxAppleClang::addSystemRootOption(outArgList, m_state);
}

/*****************************************************************************/
bool LinkerAppleClang::addSystemLibDirs(StringList& outArgList) const
{
	UNUSED(outArgList);
	return true;
}

/*****************************************************************************/
void LinkerAppleClang::addObjectiveCxxLink(StringList& outArgList) const
{
	// Unused in AppleClang
	UNUSED(outArgList);
}

}
