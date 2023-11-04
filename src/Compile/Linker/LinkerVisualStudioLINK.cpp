/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerVisualStudioLINK::LinkerVisualStudioLINK(const BuildState& inState, const SourceTarget& inProject) :
	ILinker(inState, inProject),
	m_msvcAdapter(inState, inProject)
{
}

/*****************************************************************************/
bool LinkerVisualStudioLINK::initialize()
{
	return true;
}

/*****************************************************************************/
void LinkerVisualStudioLINK::getCommandOptions(StringList& outArgList)
{
	UNUSED(outArgList);
}

/*****************************************************************************/
StringList LinkerVisualStudioLINK::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	if (m_state.toolchain.linker().empty())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.linker()));
	ret.emplace_back("/nologo");
	ret.emplace_back("/dll");

	addLinkerOptions(ret);

	{
		addIncremental(ret, outputFileBase);
		addAdditionalOptions(ret);
		addLibDirs(ret);
		addLinks(ret);
		// addPrecompiledHeaderLink(ret);
		addDebug(ret, outputFileBase);
		addSubSystem(ret);
		addLinkTimeOptimizations(ret);
		addLinkTimeCodeGeneration(ret, outputFileBase);
		addRandomizedBaseAddress(ret);
		addCompatibleWithDataExecutionPrevention(ret);
		addMachine(ret);
	}

	addWarningsTreatedAsErrors(ret);
	addEntryPoint(ret);

	ret.emplace_back(getPathCommand("/implib:", fmt::format("{}.lib", outputFileBase)));
	ret.emplace_back(getPathCommand("/out:", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
StringList LinkerVisualStudioLINK::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFile, sourceObjs);

	chalet_assert(!outputFile.empty() && sourceObjs.size() > 0, "");

	StringList ret;

	if (m_state.toolchain.linker().empty())
		return ret;

	ret.emplace_back(getQuotedPath(m_state.toolchain.linker()));
	ret.emplace_back("/nologo");

	addLinkerOptions(ret);

	{
		addIncremental(ret, outputFileBase);
		addAdditionalOptions(ret);
		addLibDirs(ret);
		addLinks(ret);
		// addPrecompiledHeaderLink(ret);
		addDebug(ret, outputFileBase);
		addSubSystem(ret);
		addLinkTimeOptimizations(ret);
		addLinkTimeCodeGeneration(ret, outputFileBase);
		addRandomizedBaseAddress(ret);
		addCompatibleWithDataExecutionPrevention(ret);
		addMachine(ret);
	}

	addWarningsTreatedAsErrors(ret);
	addEntryPoint(ret);

	ret.emplace_back(getPathCommand("/out:", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
bool LinkerVisualStudioLINK::addExecutable(StringList& outArgList) const
{
	auto& executable = m_state.toolchain.linker();
	if (executable.empty())
		return false;

	outArgList.emplace_back(getQuotedPath(executable));
	return true;
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLibDirs(StringList& outArgList) const
{
	std::string option{ "/libpath:" };
	auto libDirs = m_msvcAdapter.getLibDirectories();
	for (const auto& dir : libDirs)
	{
		outArgList.emplace_back(getPathCommand(option, dir));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinks(StringList& outArgList) const
{
	StringList links = m_msvcAdapter.getLinks();
	for (auto& link : links)
	{
		List::addIfDoesNotExist(outArgList, std::move(link));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, option);
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addProfileInformation(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsProfiling())
		List::addIfDoesNotExist(outArgList, "/profile");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addSubSystem(StringList& outArgList) const
{
	const auto subSystem = m_msvcAdapter.getSubSystem();
	if (!subSystem.empty())
		List::addIfDoesNotExist(outArgList, fmt::format("/subsystem:{}", subSystem));
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addEntryPoint(StringList& outArgList) const
{
	const auto entryPoint = m_msvcAdapter.getEntryPoint();
	if (!entryPoint.empty())
		List::addIfDoesNotExist(outArgList, fmt::format("/entry:{}", entryPoint));
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeOptimizations(StringList& outArgList) const
{
	// Note: These are also tied to /incremental (implied with /debug)
	if (m_msvcAdapter.supportsOptimizeReferences())
		List::addIfDoesNotExist(outArgList, "/opt:REF");
	else
		List::addIfDoesNotExist(outArgList, "/opt:NOREF");

	if (m_msvcAdapter.supportsCOMDATFolding())
		List::addIfDoesNotExist(outArgList, "/opt:ICF");
	else
		List::addIfDoesNotExist(outArgList, "/opt:NOICF");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addIncremental(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_msvcAdapter.supportsIncrementalLinking())
		List::addIfDoesNotExist(outArgList, "/incremental");
	else
		List::addIfDoesNotExist(outArgList, "/incremental:NO");

	if (m_msvcAdapter.suportsILKGeneration())
		outArgList.emplace_back(getPathCommand("/ilk:", fmt::format("{}.ilk", outputFileBase)));

	if (m_msvcAdapter.disableFixedBaseAddress())
		List::addIfDoesNotExist(outArgList, "/fixed:NO");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addDebug(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_msvcAdapter.enableDebugging())
	{
		if (m_msvcAdapter.supportsProfiling())
			List::addIfDoesNotExist(outArgList, "/debug:FULL");
		else
			List::addIfDoesNotExist(outArgList, "/debug");

		outArgList.emplace_back(getPathCommand("/pdb:", fmt::format("{}.pdb", outputFileBase)));
		outArgList.emplace_back(getPathCommand("/pdbstripped:", fmt::format("{}.stripped.pdb", outputFileBase)));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addRandomizedBaseAddress(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsRandomizedBaseAddress())
		List::addIfDoesNotExist(outArgList, "/dynamicbase");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addCompatibleWithDataExecutionPrevention(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsDataExecutionPrevention())
		List::addIfDoesNotExist(outArgList, "/nxcompat");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addMachine(StringList& outArgList) const
{
	auto machine = m_msvcAdapter.getMachineArchitecture();
	if (!machine.empty())
		outArgList.emplace_back(fmt::format("/machine:{}", machine));
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeCodeGeneration(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_msvcAdapter.supportsLinkTimeCodeGeneration())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		outArgList.emplace_back("/ltcg:INCREMENTAL");
		outArgList.emplace_back(fmt::format("/ltcgout:{}.iobj", outputFileBase));

		// outArgList.emplace_back(fmt::format("/pgd:{}.pgd", outputFileBase));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addWarningsTreatedAsErrors(StringList& outArgList) const
{
	if (m_project.treatWarningsAsErrors())
		outArgList.emplace_back("/WX");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addAdditionalOptions(StringList& outArgList) const
{
	auto options = m_msvcAdapter.getAdditionalLinkerOptions();
	for (auto&& option : options)
	{
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

}
