/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/ILinker.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/Linker/LinkerAppleLD.hpp"
#include "Compile/Linker/LinkerGNULD.hpp"
#include "Compile/Linker/LinkerIntelClassicLD.hpp"
#include "Compile/Linker/LinkerIntelClassicLINK.hpp"
#include "Compile/Linker/LinkerIntelLLD.hpp"
#include "Compile/Linker/LinkerLLVMLLD.hpp"
#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

namespace chalet
{
/*****************************************************************************/
ILinker::ILinker(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<ILinker> ILinker::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	LOG("ILinker:", static_cast<int>(inType), executable);
	const bool lld = String::equals("lld", executable);

#if defined(CHALET_WIN32)
	if (String::equals("link", executable))
		return std::make_unique<LinkerVisualStudioLINK>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals("xilink", executable))
		return std::make_unique<LinkerIntelClassicLINK>(inState, inProject);
	else
#elif defined(CHALET_MACOS)
	if (inType == ToolchainType::AppleLLVM)
		return std::make_unique<LinkerAppleLD>(inState, inProject);
	else if (inType == ToolchainType::IntelClassic && String::equals("xild", executable))
		return std::make_unique<LinkerIntelClassicLD>(inState, inProject);
	else
#endif
		if (lld && inType == ToolchainType::IntelLLVM)
		return std::make_unique<LinkerIntelLLD>(inState, inProject);

	if (lld || String::equals("llvm-ld", executable))
		return std::make_unique<LinkerLLVMLLD>(inState, inProject);

	return std::make_unique<LinkerGNULD>(inState, inProject);
}

/*****************************************************************************/
StringList ILinker::getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	ProjectKind kind = m_project.kind();
	if (kind == ProjectKind::SharedLibrary)
	{
		return getSharedLibTargetCommand(outputFile, sourceObjs, outputFileBase);
	}
	else if (kind == ProjectKind::Executable)
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
void ILinker::addStripSymbolsOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinkerOptions(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addProfileInformationLinkerOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addLinkTimeOptimizationOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void ILinker::addThreadModelLinkerOption(StringList& outArgList) const
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
void ILinker::addStaticCompilerLibraryOptions(StringList& outArgList) const
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

}
