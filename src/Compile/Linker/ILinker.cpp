/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/ILinker.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/Linker/LinkerAppleClang.hpp"
#include "Compile/Linker/LinkerGCC.hpp"
#include "Compile/Linker/LinkerIntelClang.hpp"
#include "Compile/Linker/LinkerIntelClassicGCC.hpp"
#include "Compile/Linker/LinkerIntelClassicLINK.hpp"
#include "Compile/Linker/LinkerLLVMClang.hpp"
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
	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	// LOG("ILinker:", static_cast<int>(inType), executable);
	const bool lld = String::equals("lld", executable);

#if defined(CHALET_WIN32)
	if (String::equals("link", executable))
		return std::make_unique<LinkerVisualStudioLINK>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals("xilink", executable))
		return std::make_unique<LinkerIntelClassicLINK>(inState, inProject);
	else
#elif defined(CHALET_MACOS)
	if (inType == ToolchainType::AppleLLVM)
		return std::make_unique<LinkerAppleClang>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals("xild", executable))
		return std::make_unique<LinkerIntelClassicGCC>(inState, inProject);
	else
#endif
		if (lld && inType == ToolchainType::IntelLLVM)
		return std::make_unique<LinkerIntelClang>(inState, inProject);

	if (lld || inType == ToolchainType::LLVM)
		return std::make_unique<LinkerLLVMClang>(inState, inProject);

	return std::make_unique<LinkerGCC>(inState, inProject);
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
	for (auto& source : sourceObjs)
	{
		outArgList.push_back(source);
	}
}

/*****************************************************************************/
StringList ILinker::getWin32Links() const
{
	// TODO: Dynamic way of determining this list
	//   would they differ between console app & windows app?
	//   or target architecture?

	StringList ret;

	ret.emplace_back("dbghelp");
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

}
