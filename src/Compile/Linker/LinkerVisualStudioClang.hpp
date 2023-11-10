/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#pragma once

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"
#include "Compile/Linker/LinkerLLVMClang.hpp"

namespace chalet
{
struct LinkerVisualStudioClang final : public LinkerLLVMClang
{
	explicit LinkerVisualStudioClang(const BuildState& inState, const SourceTarget& inProject);

protected:
	virtual void addLinks(StringList& outArgList) const final;
	virtual void addLinkerOptions(StringList& outArgList) const final;
	virtual void addProfileInformation(StringList& outArgList) const final;
	virtual void addSubSystem(StringList& outArgList) const final;
	virtual void addEntryPoint(StringList& outArgList) const final;

private:
	StringList getMSVCRuntimeLinks() const;

	CommandAdapterMSVC m_msvcAdapter;
};
}
