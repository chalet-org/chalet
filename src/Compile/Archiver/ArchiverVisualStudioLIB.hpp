/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Archiver/IArchiver.hpp"
#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

namespace chalet
{
struct ArchiverVisualStudioLIB : public IArchiver
{
	explicit ArchiverVisualStudioLIB(const BuildState& inState, const SourceTarget& inProject);

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs) const override;

protected:
	virtual void addMachine(StringList& outArgList) const;

	virtual void addLinkTimeCodeGeneration(StringList& outArgList) const;
	virtual void addWarningsTreatedAsErrors(StringList& outArgList) const;

	CommandAdapterMSVC m_msvcAdapter;
};
}
