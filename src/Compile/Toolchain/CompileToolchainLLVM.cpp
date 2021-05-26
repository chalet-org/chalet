/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "Utility/List.hpp"

// TODO: -ftime-trace

namespace chalet
{
/*****************************************************************************/
CompileToolchainLLVM::CompileToolchainLLVM(const BuildState& inState, const ProjectConfiguration& inProject, const CompilerConfig& inConfig) :
	CompileToolchainGNU(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainLLVM::type() const
{
	return ToolchainType::LLVM;
}

/*****************************************************************************/
StringList CompileToolchainLLVM::getLinkExclusions() const
{
	return { "stdc++fs" };
}

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

/*****************************************************************************/
// Compile
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addProfileInformationCompileOption(StringList& outArgList) const
{
	// TODO: "-pg" was added in a recent version of Clang (12 or 13 maybe?)
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLibStdCppCompileOption(StringList& outArgList, const CxxSpecialization specialization) const
{
	if (specialization != CxxSpecialization::ObjectiveC)
	{
		List::addIfDoesNotExist(outArgList, "-stdlib=libc++");
	}
}

/*****************************************************************************/
void CompileToolchainLLVM::addPositionIndependentCodeOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addThreadModelCompileOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addStripSymbolsOption(StringList& outArgList) const
{
	// TODO: The "-s" flag might just not exist in AppleClang
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLinkerScripts(StringList& outArgList) const
{
	// TODO: Check if there's a clang/apple clang version of this
	// If using LD with "-fuse-ld=lld-link", you can pass linkerscripts supposedly
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLibStdCppLinkerOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "-stdlib=libc++");
}

/*****************************************************************************/
void CompileToolchainLLVM::addStaticCompilerLibraryOptions(StringList& outArgList) const
{
	if (m_project.staticLinking())
	{
		List::addIfDoesNotExist(outArgList, "-static-libsan");

		// TODO: Investigate for other -static candidates on clang/mac
	}
}

/*****************************************************************************/
void CompileToolchainLLVM::addPlatformGuiApplicationFlag(StringList& outArgList) const
{
	// Noop in clang for now
	UNUSED(outArgList);
}

/*****************************************************************************/
// Linking (Misc)
/*****************************************************************************/
/*****************************************************************************/
// TOOD: I think Clang on Linux could still use the system linker (LD), in which case,
//   These flags could be used
void CompileToolchainLLVM::startStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void CompileToolchainLLVM::endStaticLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

void CompileToolchainLLVM::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
	UNUSED(outArgList);
}

}
