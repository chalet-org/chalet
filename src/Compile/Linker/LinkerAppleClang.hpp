/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/Linker/LinkerLLVMClang.hpp"

namespace chalet
{
struct LinkerAppleClang final : public LinkerLLVMClang
{
	explicit LinkerAppleClang(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs) final;

	virtual void addLinkerOptions(StringList& outArgList) const final;
	virtual void addStripSymbols(StringList& outArgList) const final;
	virtual void addThreadModelLinks(StringList& outArgList) const final;
	virtual void addProfileInformation(StringList& outArgList) const final;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const final;
	virtual void addSanitizerOptions(StringList& outArgList) const final;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const final;
	virtual bool addSystemRootOption(StringList& outArgList) const final;
	virtual bool addSystemLibDirs(StringList& outArgList) const final;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& outArgList) const final;
};
}
