/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Archiver/ArchiverVisualStudioLIB.hpp"

#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"

namespace chalet
{
/*****************************************************************************/
ArchiverVisualStudioLIB::ArchiverVisualStudioLIB(const BuildState& inState, const SourceTarget& inProject) :
	IArchiver(inState, inProject),
	m_msvcAdapter(inState, inProject)
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

	ret.emplace_back(getQuotedPath(m_state.toolchain.archiver()));
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
	auto machine = m_msvcAdapter.getMachineArchitecture();
	if (!machine.empty())
	{
		outArgList.emplace_back(fmt::format("/machine:{}", machine));
	}
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addLinkTimeCodeGeneration(StringList& outArgList) const
{
	// Requires /GL
	if (m_msvcAdapter.supportsLinkTimeCodeGeneration())
		outArgList.emplace_back("/LTCG");
}

/*****************************************************************************/
void ArchiverVisualStudioLIB::addWarningsTreatedAsErrors(StringList& outArgList) const
{
	if (m_project.treatWarningsAsErrors())
		outArgList.emplace_back("/WX");
}

}
