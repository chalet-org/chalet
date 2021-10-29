/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverVisualStudioLIB::ArchiverVisualStudioLIB(const BuildState& inState, const SourceTarget& inProject) :
	IArchiver(inState, inProject)
{
}

/*****************************************************************************/
StringList ArchiverVisualStudioLIB::getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) const
{
	chalet_assert(!outputFile.empty() && !sourceObjs.empty(), "");

	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.archiver()));
	ret.emplace_back("/nologo");

	addTargetPlatformArch(ret);

	/*if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		ret.emplace_back("/LTCG");
	}*/

	if (m_project.warningsTreatedAsErrors())
		ret.emplace_back("/WX");

	// const auto& objDir = m_state.paths.objDir();
	// ret.emplace_back(fmt::format("/DEF:{}/{}.def", objDir, outputFileBase));
	UNUSED(outputFileBase);

	// TODO: /SUBSYSTEM

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addTargetPlatformArch(StringList& outArgList) const
{
	// TODO: /MACHINE - target platform arch
	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			outArgList.emplace_back("/machine:x64");
			break;

		case Arch::Cpu::X86:
			outArgList.emplace_back("/machine:x86");
			break;

		case Arch::Cpu::ARM:
			outArgList.emplace_back("/machine:arm");
			break;

		case Arch::Cpu::ARM64:
		default:
			// ??
			break;
	}
}

}
