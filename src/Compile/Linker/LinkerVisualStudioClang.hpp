/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#ifndef CHALET_LINKER_VISUAL_STUDIO_CLANG_HPP
#define CHALET_LINKER_VISUAL_STUDIO_CLANG_HPP

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

#endif // CHALET_LINKER_VISUAL_STUDIO_CLANG_HPP
