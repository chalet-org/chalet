/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "Utility/List.hpp"

// TODO: Find a nice way to separate out the clang/appleclang stuff from CompileToolchainGNU

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
// Note: Noops mean a flag/feature isn't supported

/*****************************************************************************/
// Compile
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addProfileInformationCompileOption(StringList& inArgList) const
{
	// TODO: "-pg" was added in a recent version of Clang (12 or 13 maybe?)
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLibStdCppCompileOption(StringList& inArgList, const CxxSpecialization specialization) const
{
	if (specialization != CxxSpecialization::ObjectiveC)
	{
		List::addIfDoesNotExist(inArgList, "-stdlib=libc++");
	}
}

/*****************************************************************************/
void CompileToolchainLLVM::addPositionIndependentCodeOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addThreadModelCompileOption(StringList& inArgList) const
{
	UNUSED(inArgList);
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addStripSymbolsOption(StringList& inArgList) const
{
	// TODO: The "-s" flag might just not exist in AppleClang
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLinkerScripts(StringList& inArgList) const
{
	// TODO: Check if there's a clang/apple clang version of this
	// If using LD with "-fuse-ld=lld-link", you can pass linkerscripts supposedly
	UNUSED(inArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addLibStdCppLinkerOption(StringList& inArgList) const
{
	List::addIfDoesNotExist(inArgList, "-stdlib=libc++");
}

/*****************************************************************************/
void CompileToolchainLLVM::addStaticCompilerLibraryOptions(StringList& inArgList) const
{
	if (m_project.staticLinking())
	{
		List::addIfDoesNotExist(inArgList, "-static-libsan");

		// TODO: Investigate for other -static candidates on clang/mac
	}
}

/*****************************************************************************/
void CompileToolchainLLVM::addPlatformGuiApplicationFlag(StringList& inArgList) const
{
	// Noop in clang for now
	UNUSED(inArgList);
}

/*****************************************************************************/
// Linking (Misc)
/*****************************************************************************/
/*****************************************************************************/
// TOOD: I think Clang on Linux could still use the system linker (LD), in which case,
//   These flags could be used
void CompileToolchainLLVM::startStaticLinkGroup(StringList& inArgList) const
{
	UNUSED(inArgList);
}

void CompileToolchainLLVM::endStaticLinkGroup(StringList& inArgList) const
{
	UNUSED(inArgList);
}

void CompileToolchainLLVM::startExplicitDynamicLinkGroup(StringList& inArgList) const
{
	UNUSED(inArgList);
}

}
