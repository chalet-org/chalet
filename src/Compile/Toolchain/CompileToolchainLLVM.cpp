/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Toolchain/CompileToolchainLLVM.hpp"

#include "Compile/CompilerConfig.hpp"
#include "Compile/CompilerConfigController.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

// TODO: -ftime-trace

namespace chalet
{
/*****************************************************************************/
CompileToolchainLLVM::CompileToolchainLLVM(const BuildState& inState, const SourceTarget& inProject, const CompilerConfig& inConfig) :
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
StringList CompileToolchainLLVM::getWarningExclusions() const
{
	return {
		"noexcept",
		"strict-null-sentinel"
	};
}

/*****************************************************************************/
// Note: Noops mean a flag/feature isn't supported

/*****************************************************************************/
// Compile
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addWarnings(StringList& outArgList) const
{
	CompileToolchainGNU::addWarnings(outArgList);

	if (m_state.compilers.isWindowsClang())
	{
		const std::string prefix{ "-W" };
		std::string noLangExtensions{ "no-language-extension-token" };
		if (!List::contains(m_project.warnings(), noLangExtensions))
			outArgList.emplace_back(prefix + noLangExtensions);
	}
}

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
#if defined(CHALET_LINUX)
	// if (!m_state.compilers.isMingw())
	{
		std::string fpic{ "-fPIC" };
		// if (isFlagSupported(fpic))
		List::addIfDoesNotExist(outArgList, std::move(fpic));
	}
#else
	UNUSED(outArgList);
#endif
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

	const auto& targetArchString = m_state.info.targetArchitectureTriple();

	outArgList.emplace_back("-target");
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
		outArgList.emplace_back(fmt::format("-mcpu={}", archOptions[0]));
		outArgList.emplace_back(fmt::format("-mfpu={}", archOptions[1]));
		outArgList.emplace_back(fmt::format("-mfloat-abi={}", archOptions[2]));
	}

	return true;
}

/*****************************************************************************/
// Linking
/*****************************************************************************/
/*****************************************************************************/
void CompileToolchainLLVM::addLinks(StringList& outArgList) const
{
	CompileToolchainGNU::addLinks(outArgList);

	if (m_state.compilers.isWindowsClang())
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
void CompileToolchainLLVM::addStripSymbolsOption(StringList& outArgList) const
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
void CompileToolchainLLVM::addDiagnosticColorOption(StringList& outArgList) const
{
	std::string diagnosticColor{ "-fcolor-diagnostics" };
	if (isFlagSupported(diagnosticColor))
		List::addIfDoesNotExist(outArgList, std::move(diagnosticColor));
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
void CompileToolchainLLVM::addSubSystem(StringList& outArgList) const
{
	if (m_state.compilers.isWindowsClang())
	{
		const ProjectKind kind = m_project.kind();
		if (kind == ProjectKind::Executable)
		{
			const auto subSystem = getMsvcCompatibleSubSystem();
			List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/subsystem:{}", subSystem));
		}
	}
}

/*****************************************************************************/
void CompileToolchainLLVM::addEntryPoint(StringList& outArgList) const
{
	if (m_state.compilers.isWindowsClang())
	{
		const auto entryPoint = getMsvcCompatibleEntryPoint();
		if (!entryPoint.empty())
		{
			List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/entry:{}", entryPoint));
		}
	}
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
