/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerVisualStudioClang.hpp"

#include "State/BuildState.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerVisualStudioClang::LinkerVisualStudioClang(const BuildState& inState, const SourceTarget& inProject) :
	LinkerLLVMClang(inState, inProject),
	m_msvcAdapter(inState, inProject)
{
}

/*****************************************************************************/
void LinkerVisualStudioClang::addLinks(StringList& outArgList) const
{
	const std::string prefix{ "-l" };
	auto runtimeLinks = getMSVCRuntimeLinks();
	for (const auto& link : runtimeLinks)
	{
		List::addIfDoesNotExist(outArgList, fmt::format("{}{}", prefix, link));
	}

	LinkerLLVMClang::addLinks(outArgList);
}

/*****************************************************************************/
void LinkerVisualStudioClang::addLinkerOptions(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, std::string("-Wl,/nodefaultlib:libcmt"));

	// Note: These are also tied to /incremental (implied with /debug)
	if (m_msvcAdapter.supportsOptimizeReferences())
		List::addIfDoesNotExist(outArgList, "-Wl,/opt:REF");
	else
		List::addIfDoesNotExist(outArgList, "-Wl,/opt:NOREF");

	if (m_msvcAdapter.supportsCOMDATFolding())
		List::addIfDoesNotExist(outArgList, "-Wl,/opt:ICF");
	else
		List::addIfDoesNotExist(outArgList, "-Wl,/opt:NOICF");

	if (m_msvcAdapter.supportsIncrementalLinking())
		List::addIfDoesNotExist(outArgList, "-Wl,/incremental");
	else
		List::addIfDoesNotExist(outArgList, "-Wl,/incremental:NO");

	if (m_msvcAdapter.suportsILKGeneration())
		outArgList.emplace_back(getPathCommand("-Wl,/ilk:", fmt::format("{}.ilk", m_outputFileBase)));

	if (m_msvcAdapter.disableFixedBaseAddress())
		List::addIfDoesNotExist(outArgList, "-Wl,/fixed:NO");

	if (m_msvcAdapter.enableDebugging())
	{
		if (m_msvcAdapter.supportsProfiling())
			List::addIfDoesNotExist(outArgList, "-Wl,/debug:FULL");
		else
			List::addIfDoesNotExist(outArgList, "-Wl,/debug");

		if (!m_outputFileBase.empty())
		{
			outArgList.emplace_back(getPathCommand("-Wl,/pdb:", fmt::format("{}.pdb", m_outputFileBase)));
			outArgList.emplace_back(getPathCommand("-Wl,/pdbstripped:", fmt::format("{}.stripped.pdb", m_outputFileBase)));
		}
	}

	if (m_msvcAdapter.supportsRandomizedBaseAddress())
		List::addIfDoesNotExist(outArgList, "-Wl,/dynamicbase");

	if (m_msvcAdapter.supportsDataExecutionPrevention())
		List::addIfDoesNotExist(outArgList, "-Wl,/nxcompat");

	auto machine = m_msvcAdapter.getMachineArchitecture();
	if (!machine.empty())
		outArgList.emplace_back(fmt::format("-Wl,/machine:{}", machine));

	if (!m_outputFileBase.empty() && m_msvcAdapter.supportsLinkTimeCodeGeneration())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		outArgList.emplace_back("-Wl,/ltcg:INCREMENTAL");
		outArgList.emplace_back(fmt::format("-Wl,/ltcgout:{}.iobj", m_outputFileBase));

		// outArgList.emplace_back(fmt::format("/pgd:{}.pgd", m_outputFileBase));
	}

	auto options = m_msvcAdapter.getAdditionalLinkerOptions();
	for (auto&& option : options)
	{
		List::addIfDoesNotExist(outArgList, fmt::format("-Wl,{}", option));
	}

	LinkerLLVMClang::addLinkerOptions(outArgList);
}

/*****************************************************************************/
void LinkerVisualStudioClang::addProfileInformation(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsProfiling())
		List::addIfDoesNotExist(outArgList, "-Wl,/profile");
}

/*****************************************************************************/
void LinkerVisualStudioClang::addSubSystem(StringList& outArgList) const
{
	const SourceKind kind = m_project.kind();
	if (kind == SourceKind::Executable)
	{
		const auto subSystem = m_msvcAdapter.getSubSystem();
		List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/subsystem:{}", subSystem));
	}
}

/*****************************************************************************/
void LinkerVisualStudioClang::addEntryPoint(StringList& outArgList) const
{
	const auto entryPoint = m_msvcAdapter.getEntryPoint();
	if (!entryPoint.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("-Wl,/entry:{}", entryPoint));
	}
}

/*****************************************************************************/
StringList LinkerVisualStudioClang::getMSVCRuntimeLinks() const
{
	StringList ret;

	auto crtType = m_msvcAdapter.getRuntimeLibraryType();

	// https://devblogs.microsoft.com/cppblog/introducing-the-universal-crt/

	switch (crtType)
	{
		case WindowsRuntimeLibraryType::MultiThreadedDLL: {
			ret.emplace_back("msvcrt");
			ret.emplace_back("vcruntime");
			ret.emplace_back("ucrt");
			break;
		}
		case WindowsRuntimeLibraryType::MultiThreadedDebugDLL: {
			ret.emplace_back("msvcrtd");
			ret.emplace_back("vcruntimed");
			ret.emplace_back("ucrtd");
			break;
		}
		case WindowsRuntimeLibraryType::MultiThreaded: {
			ret.emplace_back("libcmt");
			ret.emplace_back("libvcruntime");
			ret.emplace_back("libucrt");
			break;
		}
		case WindowsRuntimeLibraryType::MultiThreadedDebug: {
			ret.emplace_back("libcmtd");
			ret.emplace_back("libvcruntimed");
			ret.emplace_back("libucrtd");
			break;
		}
		default: break;
	}

	return ret;
}

}
