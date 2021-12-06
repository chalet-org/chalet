/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_VISUAL_STUDIO_LINK_HPP
#define CHALET_LINKER_VISUAL_STUDIO_LINK_HPP

#include "Compile/Linker/ILinker.hpp"

namespace chalet
{
struct LinkerVisualStudioLINK : public ILinker
{
	explicit LinkerVisualStudioLINK(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;

	static std::string getMsvcCompatibleSubSystem(const SourceTarget& inProject);
	static std::string getMsvcCompatibleEntryPoint(const SourceTarget& inProject);

protected:
	virtual StringList getLinkExclusions() const override;
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;
	virtual StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

	virtual void addLibDirs(StringList& outArgList) const override;
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addLinkerOptions(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const override;

	// General
	virtual void addIncremental(StringList& outArgList, const std::string& outputFileBase) const;
	virtual void addDebug(StringList& outArgList, const std::string& outputFileBase) const;
	virtual void addCgThreads(StringList& outArgList) const;
	virtual void addDynamicBase(StringList& outArgList) const;
	virtual void addCompatibleWithDataExecutionPrevention(StringList& outArgList) const;
	virtual void addMachine(StringList& outArgList) const;
	virtual void addLinkTimeCodeGeneration(StringList& outArgList, const std::string& outputFileBase) const;
	virtual void addVerbosity(StringList& outArgList) const;
	virtual void addWarningsTreatedAsErrors(StringList& outArgList) const;

private:
	virtual void addUnsortedOptions(StringList& outArgList) const final;
};
}

#endif // CHALET_LINKER_VISUAL_STUDIO_LINK_HPP
