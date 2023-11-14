/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/ILinker.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/Linker/LinkerAppleClang.hpp"
#include "Compile/Linker/LinkerEmscripten.hpp"
#include "Compile/Linker/LinkerGCC.hpp"
#include "Compile/Linker/LinkerIntelClang.hpp"
#include "Compile/Linker/LinkerIntelClassicGCC.hpp"
#include "Compile/Linker/LinkerIntelClassicLINK.hpp"
#include "Compile/Linker/LinkerLLVMClang.hpp"
#include "Compile/Linker/LinkerVisualStudioClang.hpp"
#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

namespace chalet
{
/*****************************************************************************/
ILinker::ILinker(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
[[nodiscard]] Unique<ILinker> ILinker::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto exec = String::toLowerCase(String::getPathBaseName(inExecutable));
	// LOG("ILinker:", static_cast<i32>(inType), exec, inExecutable);

	auto linkerMatches = [&exec](const char* id, const bool typeMatches, const char* label, const bool failTypeMismatch = true, const bool onlyType = true) -> i32 {
		return executableMatches(exec, "linker", id, typeMatches, label, failTypeMismatch, onlyType);
	};

	// The goal here is to not only return the correct linker,
	//   but validate the linker with the toolchain type

#if defined(CHALET_WIN32)
	if (i32 result = linkerMatches("link", inType == ToolchainType::VisualStudio, "Visual Studio", false, false); result >= 0)
		return makeTool<LinkerVisualStudioLINK>(result, inState, inProject);

	if (i32 result = linkerMatches("xilink", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
		return makeTool<LinkerIntelClassicLINK>(result, inState, inProject);

	if (i32 result = linkerMatches("lld", inType == ToolchainType::LLVM || inType == ToolchainType::VisualStudioLLVM, "LLVM", false); result >= 0)
		return makeTool<LinkerVisualStudioClang>(result, inState, inProject);

	// if (i32 result = linkerMatches("ld", inType == ToolchainType::MingwLLVM, "LLVM", false, false); result >= 0)
	// 	return makeTool<LinkerGCC>(result, inState, inProject);

	if (i32 result = linkerMatches("lld", inType == ToolchainType::MingwLLVM, "LLVM", false); result >= 0)
	{
		if (result == 1)
			return makeTool<LinkerLLVMClang>(result, inState, inProject);
		else
			Diagnostic::clearErrors();
	}

#elif defined(CHALET_MACOS)
	if (i32 result = linkerMatches("ld", inType == ToolchainType::AppleLLVM, "AppleClang", false); result >= 0)
		return makeTool<LinkerAppleClang>(result, inState, inProject);

	if (i32 result = linkerMatches("xild", inType == ToolchainType::IntelClassic, "Intel Classic"); result >= 0)
		return makeTool<LinkerIntelClassicGCC>(result, inState, inProject);

#endif

#if !defined(CHALET_WIN32)
	if (i32 result = linkerMatches("lld", inType == ToolchainType::LLVM, "LLVM", false); result >= 0)
		return makeTool<LinkerLLVMClang>(result, inState, inProject);
#endif

	if (i32 result = linkerMatches("lld", inType == ToolchainType::IntelLLVM, "Intel LLVM"); result >= 0)
		return makeTool<LinkerIntelClang>(result, inState, inProject);

	if (String::equals("lld", exec))
	{
		Diagnostic::error("Found 'lld' in a toolchain other than LLVM");
		return nullptr;
	}

	if (i32 result = linkerMatches("wasm-ld", inType == ToolchainType::Emscripten, "Emscripten"); result >= 0)
		return makeTool<LinkerEmscripten>(result, inState, inProject);

	return std::make_unique<LinkerGCC>(inState, inProject);
}

/*****************************************************************************/
[[nodiscard]] StringList ILinker::getWin32CoreLibraryLinks(const BuildState& inState, const SourceTarget& inProject)
{
	// TODO: Dynamic way of determining this list
	//   would they differ between console app & windows app?
	//   or target architecture?

	StringList ret;

	u32 versionMajorMinor = inState.toolchain.compilerCxx(inProject.language()).versionMajorMinor;
	bool oldMinGW = inState.environment->isMingwGcc() && versionMajorMinor < 700;
	if (!oldMinGW)
	{
		ret.emplace_back("dbghelp");
	}

	ret.emplace_back("kernel32");
	ret.emplace_back("user32");
	ret.emplace_back("gdi32");
	ret.emplace_back("winspool");
	ret.emplace_back("shell32");
	ret.emplace_back("ole32");
	ret.emplace_back("oleaut32");
	ret.emplace_back("uuid");
	ret.emplace_back("comdlg32");
	ret.emplace_back("advapi32");

	// imm32
	// setupapi
	// version
	// winmm

	return ret;
}

/*****************************************************************************/
StringList ILinker::getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	SourceKind kind = m_project.kind();
	if (kind == SourceKind::SharedLibrary)
	{
		return getSharedLibTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
	else if (kind == SourceKind::Executable)
	{
		return getExecutableTargetCommand(outputFile, sourceObjs, outputFileBase);
	}

	return {};
}

/*****************************************************************************/
void ILinker::addLibDirs(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addRunPath(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addStripSymbols(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinkerOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addProfileInformation(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinkTimeOptimizations(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addThreadModelLinks(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinkerScripts(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLibStdCppLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addSanitizerOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addStaticCompilerLibraries(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addSubSystem(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addEntryPoint(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
bool ILinker::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	UNUSED(outArgList, inArch);
	return true;
}

/*****************************************************************************/
void ILinker::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	auto strategy = m_state.toolchain.strategy();
	if (strategy == StrategyType::Ninja || strategy == StrategyType::Makefile)
	{
		for (auto& source : sourceObjs)
		{
			outArgList.emplace_back(source);
		}
	}
	else
	{
		for (auto& source : sourceObjs)
		{
			outArgList.emplace_back(getQuotedPath(source));
		}
	}
}

/*****************************************************************************/
StringList ILinker::getWin32CoreLibraryLinks() const
{
	return ILinker::getWin32CoreLibraryLinks(m_state, m_project);
}
}
