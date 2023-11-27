/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/Linker/ILinker.hpp"

namespace chalet
{
struct LinkerVisualStudioLINK : public ILinker
{
	explicit LinkerVisualStudioLINK(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	virtual void getCommandOptions(StringList& outArgList) override;

protected:
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs) override;
	virtual StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs) override;

	virtual bool addExecutable(StringList& outArgList) const override;
	virtual void addLibDirs(StringList& outArgList) const override;
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addLinkerOptions(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const override;

	// General
	virtual void addIncremental(StringList& outArgList) const;
	virtual void addDebug(StringList& outArgList) const;
	virtual void addRandomizedBaseAddress(StringList& outArgList) const;
	virtual void addCompatibleWithDataExecutionPrevention(StringList& outArgList) const;
	virtual void addMachine(StringList& outArgList) const;
	virtual void addLinkTimeCodeGeneration(StringList& outArgList) const;
	virtual void addWarningsTreatedAsErrors(StringList& outArgList) const;
	virtual void addAdditionalOptions(StringList& outArgList) const;

private:
	CommandAdapterMSVC m_msvcAdapter;
};
}
