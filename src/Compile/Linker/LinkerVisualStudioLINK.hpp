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
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	// General
	virtual void addCgThreads(StringList& outArgList) const;

private:
	void addPrecompiledHeaderLink(StringList outArgList) const;
};
}

#endif // CHALET_LINKER_VISUAL_STUDIO_LINK_HPP