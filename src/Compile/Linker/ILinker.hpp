/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/IToolchainExecutableBase.hpp"

namespace chalet
{
struct ILinker : public IToolchainExecutableBase
{
	explicit ILinker(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<ILinker> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);
	[[nodiscard]] static StringList getWin32CoreLibraryLinks(const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() = 0;

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs) final;
	virtual void getCommandOptions(StringList& outArgList) = 0;

protected:
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs) = 0;
	virtual StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs) = 0;

	virtual bool addExecutable(StringList& outArgList) const = 0;

	virtual void addLibDirs(StringList& outArgList) const;
	virtual void addLinks(StringList& outArgList) const;
	virtual void addRunPath(StringList& outArgList) const;
	virtual void addStripSymbols(StringList& outArgList) const;
	virtual void addLinkerOptions(StringList& outArgList) const;
	virtual void addProfileInformation(StringList& outArgList) const;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const;
	virtual void addThreadModelLinks(StringList& outArgList) const;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const;
	virtual void addSanitizerOptions(StringList& outArgList) const;
	virtual void addStaticCompilerLibraries(StringList& outArgList) const;
	virtual void addSubSystem(StringList& outArgList) const;
	virtual void addEntryPoint(StringList& outArgList) const;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const;

	virtual void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const final;

	StringList getWin32CoreLibraryLinks() const;
	const std::string& outputFileBase() const;

	u32 m_versionMajorMinor = 0;
	u32 m_versionPatch = 0;

private:
	mutable std::string m_outputFileBase;
};
}
