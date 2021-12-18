/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_ILINKER_HPP
#define CHALET_ILINKER_HPP

#include "Compile/IToolchainExecutableBase.hpp"

namespace chalet
{
struct ILinker : public IToolchainExecutableBase
{
	explicit ILinker(const BuildState& inState, const SourceTarget& inProject);

	[[nodiscard]] static Unique<ILinker> make(const ToolchainType inType, const std::string& inExecutable, const BuildState& inState, const SourceTarget& inProject);

	virtual bool initialize() = 0;

	virtual StringList getCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) final;

protected:
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) = 0;
	virtual StringList getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) = 0;

	virtual void addLibDirs(StringList& outArgList) const;
	virtual void addLinks(StringList& outArgList) const;
	virtual void addRunPath(StringList& outArgList) const;
	virtual void addStripSymbols(StringList& outArgList) const;
	virtual void addLinkerOptions(StringList& outArgList) const;
	virtual void addProfileInformation(StringList& outArgList) const;
	virtual void addLinkTimeOptimizations(StringList& outArgList) const;
	virtual void addThreadModelLinks(StringList& outArgList) const;
	virtual void addLinkerScripts(StringList& outArgList) const;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const;
	virtual void addSanitizerOptions(StringList& outArgList) const;
	virtual void addStaticCompilerLibraries(StringList& outArgList) const;
	virtual void addSubSystem(StringList& outArgList) const;
	virtual void addEntryPoint(StringList& outArgList) const;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const;

	virtual void addSourceObjects(StringList& outArgList, const StringList& sourceObjs) const final;

	uint m_versionMajorMinor = 0;
	uint m_versionPatch = 0;
};
}

#endif // CHALET_ILINKER_HPP
