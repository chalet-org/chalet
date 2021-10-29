/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/IArchiver.hpp"

#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/String.hpp"

#include "Compile/Archiver/ArchiverGNUAR.hpp"
#include "Compile/Archiver/ArchiverIntelClassicAR.hpp"
#include "Compile/Archiver/ArchiverIntelClassicLIB.hpp"
#include "Compile/Archiver/ArchiverLLVMAR.hpp"
#include "Compile/Archiver/ArchiverLibTool.hpp"
#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

namespace chalet
{
/*****************************************************************************/
IArchiver::IArchiver(const BuildState& inState, const SourceTarget& inProject) :
	IToolchainExecutableBase(inState, inProject)
{
}

/*****************************************************************************/
[[nodiscard]] Unique<IArchiver> IArchiver::make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject)
{
	UNUSED(inType);
	const auto executable = String::toLowerCase(String::getPathFolderBaseName(String::getPathFilename(inExecutable)));
	LOG("IArchiver:", static_cast<int>(inType), executable);
#if defined(CHALET_WIN32)
	if (String::equals("lib", executable))
		return std::make_unique<ArchiverVisualStudioLIB>(inState, inProject);
	else if (String::equals("xilib", executable))
		return std::make_unique<ArchiverIntelClassicLIB>(inState, inProject);
	else
#elif defined(CHALET_MACOS)
	if (String::equals("libtool", executable))
		return std::make_unique<ArchiverLibTool>(inState, inProject);
	else if (String::equals("xiar", executable))
		return std::make_unique<ArchiverIntelClassicAR>(inState, inProject);
	else
#endif
		if (String::equals("llvm-ar", executable))
		return std::make_unique<ArchiverLLVMAR>(inState, inProject);

	return std::make_unique<ArchiverGNUAR>(inState, inProject);
}

/*****************************************************************************/
void IArchiver::addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const
{
	for (auto& source : sourceObjs)
	{
		outArgList.push_back(source);
	}
}
}
