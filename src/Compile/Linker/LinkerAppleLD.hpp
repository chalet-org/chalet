/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_APPLE_LLD_HPP
#define CHALET_LINKER_APPLE_LLD_HPP

#include "Compile/Linker/LinkerLLVMLLD.hpp"

namespace chalet
{
struct LinkerAppleLD final : public LinkerLLVMLLD
{
	explicit LinkerAppleLD(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual StringList getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase) final;

	virtual void addStripSymbols(StringList& outArgList) const final;
	virtual void addThreadModelLinks(StringList& outArgList) const final;
	virtual void addProfileInformationLinkerOption(StringList& outArgList) const final;
	virtual void addLibStdCppLinkerOption(StringList& outArgList) const final;
	virtual bool addArchitecture(StringList& outArgList, const std::string& inArch) const final;

	// Objective-C / Objective-C++
	virtual void addObjectiveCxxLink(StringList& outArgList) const final;
};
}

#endif // CHALET_LINKER_APPLE_LLD_HPP
