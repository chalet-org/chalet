/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerAppleClang.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
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

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedPath(executable));

	ret.emplace_back("-dynamiclib");
	// ret.emplace_back("-flat_namespace");

	addStripSymbols(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformation(ret);
	addLinkTimeOptimizations(ret);
	addThreadModelLinks(ret);
	addArchitecture(ret, std::string());

	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addSanitizerOptions(ret);
	addStaticCompilerLibraries(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addMacosFrameworkOptions(ret);
	addRunPath(ret);

	addLibDirs(ret);

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
		std::string flag{ "-stdlib=libc++" };
		// if (isFlagSupported(flag))
		List::addIfDoesNotExist(outArgList, std::move(flag));

		// TODO: Apple has a "-stdlib=libstdc++" flag that is pre-C++11 for compatibility
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
	if (m_state.info.targetArchitecture() != Arch::Cpu::UniversalMacOS)
#endif
	{
		if (!LinkerLLVMClang::addArchitecture(outArgList, inArch))
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
void LinkerAppleClang::addObjectiveCxxLink(StringList& outArgList) const
{
	// Unused in AppleClang
	UNUSED(outArgList);
}

}
