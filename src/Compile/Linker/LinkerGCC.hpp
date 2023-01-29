/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_GCC_HPP
#define CHALET_LINKER_GCC_HPP

#include "Compile/Linker/ILinker.hpp"

namespace chalet
{
struct LinkerGCC : public ILinker
{
	explicit LinkerGCC(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() override;
	virtual void getCommandOptions(StringList& outArgList) override;

protected:
	virtual bool isLinkSupported(const std::string& inLink) const;

	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;
	virtual StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) override;

	virtual void addLibDirs(StringList& outArgList) const override;
	virtual void addLinks(StringList& outArgList) const override;
	virtual void addRunPath(StringList& outArgList) const override;
	virtual void addStripSymbols(StringList& outArgList) const override;
	virtual void addLinkerOptions(StringList& outArgList) const override;
	virtual void addProfileInformation(StringList& outArgList) const override;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const override;
	virtual void addThreadModelLinks(StringList& outArgList) const override;
	virtual void addLinkerScripts(StringList& outArgList) const override;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const override;
	virtual void addSanitizerOptions(StringList& outArgList) const override;
	virtual void addStaticCompilerLibraries(StringList& outArgList) const override;
	virtual void addSubSystem(StringList& outArgList) const override;
	virtual void addEntryPoint(StringList& outArgList) const override;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const override;

	// Linking (Misc)
	virtual void addCppFilesystem(StringList& outArgList) const;
	virtual void startStaticLinkGroup(StringList& outArgList) const;
	virtual void endStaticLinkGroup(StringList& outArgList) const;
	virtual void startExplicitDynamicLinkGroup(StringList& outArgList) const;
	virtual void addCompilerSearchPaths(StringList& outArgList) const;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& outArgList) const;

	// MacOS
	virtual void addMacosFrameworkOptions(StringList& outArgList) const;

	// GNU GCC stuff
	virtual bool addSystemRootOption(StringList& outArgList) const;
	virtual bool addSystemLibDirs(StringList& outArgList) const;
	virtual void addPositionIndependentCodeOption(StringList& outArgList) const;

	std::string m_outputFileBase;

private:
	// void initializeSupportedLinks();

	Dictionary<bool> m_supportedLinks;
};
}

#endif // CHALET_LINKER_GCC_HPP
