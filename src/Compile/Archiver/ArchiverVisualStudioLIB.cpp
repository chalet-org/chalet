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
	UNUSED(outputFileBase);

	chalet_assert(!outputFile.empty() && !sourceObjs.empty(), "");

	StringList ret;

	if (m_state.toolchain.archiver().empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.archiver()));
	ret.emplace_back("/nologo");

	addMachine(ret);
	addLinkTimeCodeGeneration(ret);
	addWarningsTreatedAsErrors(ret);

	ret.emplace_back(getPathCommand("/out:", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addMachine(StringList& outArgList) const
{
	// TODO: /MACHINE - target platform arch
	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			outArgList.emplace_back("/machine:X64");
			break;

		case Arch::Cpu::X86:
			outArgList.emplace_back("/machine:X86");
			break;

		case Arch::Cpu::ARM:
			outArgList.emplace_back("/machine:ARM");
			break;

		case Arch::Cpu::ARM64:
		default:
			// ??
			break;
	}
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addLinkTimeCodeGeneration(StringList& outArgList) const
{
	// Requires /GL
	/*if (m_state.configuration.linkTimeOptimization())
	{
		ret.emplace_back("/LTCG");
	}*/
	UNUSED(outArgList);
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addWarningsTreatedAsErrors(StringList& outArgList) const
{
	if (m_project.warningsTreatedAsErrors())
		outArgList.emplace_back("/WX");
}

}
