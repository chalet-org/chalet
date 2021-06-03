/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "Libraries/Format.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// TODO: -ftime-trace

namespace chalet
{
/*****************************************************************************/
CompileToolchainLLVM::CompileToolchainLLVM(const BuildState& inState, const ProjectTarget& inProject, const CompilerConfig& inConfig) :
	CompileToolchainGNU(inState, inProject, inConfig)
{
}

/*****************************************************************************/
ToolchainType CompileToolchainLLVM::type() const noexcept
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
StringList CompileToolchainLLVM::getDynamicLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	StringList ret;

	addExectuable(ret, m_config.compilerExecutable());

	ret.push_back("-dynamiclib");
	// ret.push_back("-fPIC");
	// ret.push_back("-flat_namespace");

	UNUSED(outputFileBase);

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret);
	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addPlatformGuiApplicationFlag(ret);
	addMacosFrameworkOptions(ret);

	addLibDirs(ret);

	ret.push_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

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
bool CompileToolchainLLVM::addArchitecture(StringList& outArgList) const
{
	UNUSED(outArgList);

	// https://clang.llvm.org/docs/CrossCompilation.html
	// TOOD: -mcpu, -mfpu, -mfloat

	// clang -print-supported-cpus

	auto hostArch = m_state.info.hostArchitecture();
	auto targetArch = m_state.info.targetArchitecture();

	if (hostArch == targetArch && targetArch != Arch::Cpu::Unknown)
		return false;

	const auto& targetArchString = m_state.info.targetArchitectureString();

	outArgList.push_back("-target");
	outArgList.push_back(targetArchString);

	return true;
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
