/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "Compile/CompilerConfig.hpp"

#include "State/BuildState.hpp"
#include "State/Target/ProjectTarget.hpp"
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
	UNUSED(outArgList, specialization);
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
	// https://clang.llvm.org/docs/CrossCompilation.html

	// clang -print-supported-cpus

	Arch::Cpu hostArch = m_state.info.hostArchitecture();
	Arch::Cpu targetArch = m_state.info.targetArchitecture();
	const auto& archOptions = m_state.info.archOptions();

	if (hostArch == targetArch && targetArch != Arch::Cpu::Unknown && archOptions.empty())
		return false;

	const auto& targetArchString = m_state.info.targetArchitectureString();

	outArgList.push_back("-target");
	outArgList.push_back(targetArchString);

	return true;
}

/*****************************************************************************/
bool CompileToolchainLLVM::addArchitectureOptions(StringList& outArgList) const
{
	// https://clang.llvm.org/docs/CrossCompilation.html

	Arch::Cpu hostArch = m_state.info.hostArchitecture();
	Arch::Cpu targetArch = m_state.info.targetArchitecture();
	const auto& archOptions = m_state.info.archOptions();

	if (hostArch == targetArch && targetArch != Arch::Cpu::Unknown && archOptions.empty())
		return false;

	if (!archOptions.empty() && archOptions.size() == 3) // <cpu-name>,<fpu-name>,<fabi>
	{
		outArgList.push_back(fmt::format("-mcpu={}", archOptions[0]));
		outArgList.push_back(fmt::format("-mfpu={}", archOptions[1]));
		outArgList.push_back(fmt::format("-mfloat-abi={}", archOptions[2]));
	}

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
void CompileToolchainLLVM::addThreadModelLinkerOption(StringList& outArgList) const
{
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
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompileToolchainLLVM::addStaticCompilerLibraryOptions(StringList& outArgList) const
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
