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

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.linker()));
	ret.emplace_back("/nologo");
	ret.emplace_back("/dll");

	addLinkerOptions(ret);

	{
		addIncremental(ret, outputFileBase);
		addLibDirs(ret);
		addLinks(ret);
		// addPrecompiledHeaderLink(ret);
		addDebug(ret, outputFileBase);
		addSubSystem(ret);
		addLinkTimeOptimizations(ret);
		addLinkTimeCodeGeneration(ret, outputFileBase);
		addUnsortedOptions(ret);
		addDynamicBase(ret);
		addCompatibleWithDataExecutionPrevention(ret);
		addMachine(ret);
	}

	addWarningsTreatedAsErrors(ret);
	addEntryPoint(ret);
	// addCgThreads(ret);
	// addVerbosity(ret);

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

	ret.emplace_back(getQuotedExecutablePath(m_state.toolchain.linker()));
	ret.emplace_back("/nologo");

	addLinkerOptions(ret);

	{
		addIncremental(ret, outputFileBase);
		addLibDirs(ret);
		addLinks(ret);
		// addPrecompiledHeaderLink(ret);
		addDebug(ret, outputFileBase);
		addSubSystem(ret);
		addLinkTimeOptimizations(ret);
		addLinkTimeCodeGeneration(ret, outputFileBase);
		addUnsortedOptions(ret);
		addDynamicBase(ret);
		addCompatibleWithDataExecutionPrevention(ret);
		addMachine(ret);
	}

	addWarningsTreatedAsErrors(ret);
	addEntryPoint(ret);
	// addCgThreads(ret);
	// addVerbosity(ret);

	ret.emplace_back(getPathCommand("/out:", outputFile));

	addSourceObjects(ret, sourceObjs);

	return ret;
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLibDirs(StringList& outArgList) const
{
	std::string option{ "/libpath:" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(option, dir));
	}

	outArgList.emplace_back(getPathCommand(option, m_state.paths.buildOutputDir()));
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinks(StringList& outArgList) const
{
	// const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	// const bool hasDynamicLinks = m_project.links().size() > 0;

	StringList links = m_project.links();
	for (auto& link : m_project.staticLinks())
	{
		links.push_back(link);
	}

	for (auto& link : links)
	{
		bool found = false;
		for (auto& target : m_state.targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);
				if (project.name() == link && project.isSharedLibrary())
				{
					auto outputFile = project.outputFile();
					if (String::endsWith(".dll", outputFile))
					{
						String::replaceAll(outputFile, ".dll", ".lib");
						List::addIfDoesNotExist(outArgList, std::move(outputFile));
						found = true;
						break;
					}
				}
			}
		}

		if (!found)
		{
			List::addIfDoesNotExist(outArgList, fmt::format("{}.lib", link));
		}
	}

	auto win32Links = getWin32Links();
	for (const auto& link : win32Links)
	{
		List::addIfDoesNotExist(outArgList, fmt::format("{}.lib", link));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		// if (isFlagSupported(option))
		outArgList.emplace_back(option);
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addProfileInformation(StringList& outArgList) const
{
	if (m_state.configuration.enableProfiling())
	{
		List::addIfDoesNotExist(outArgList, "/profile");
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addSubSystem(StringList& outArgList) const
{
	const auto subSystem = m_msvcAdapter.getSubSystem();
	if (!subSystem.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/subsystem:{}", subSystem));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addEntryPoint(StringList& outArgList) const
{
	const auto entryPoint = m_msvcAdapter.getEntryPoint();
	if (!entryPoint.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/entry:{}", entryPoint));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeOptimizations(StringList& outArgList) const
{
	const auto arch = m_state.info.targetArchitecture();
	bool isArm = arch == Arch::Cpu::ARM || arch == Arch::Cpu::ARM64;

	// Note: These are also tied to /incremental (implied with /debug)
	if (m_state.configuration.debugSymbols())
	{
		if (m_state.configuration.enableProfiling())
			List::addIfDoesNotExist(outArgList, "/opt:REF");
		else
			List::addIfDoesNotExist(outArgList, "/opt:NOREF");

		List::addIfDoesNotExist(outArgList, "/opt:NOICF");

		if (isArm)
			List::addIfDoesNotExist(outArgList, "/opt:NOLBR");
	}
	else
	{
		List::addIfDoesNotExist(outArgList, "/opt:REF");
		List::addIfDoesNotExist(outArgList, "/opt:ICF");

		if (isArm)
			List::addIfDoesNotExist(outArgList, "/opt:LBR"); // relates to arm binaries
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addIncremental(StringList& outArgList, const std::string& outputFileBase) const
{
	const bool enableProfiling = m_state.configuration.enableProfiling();
	if (m_state.configuration.debugSymbols() && !m_state.configuration.enableSanitizers() && !enableProfiling)
	{
		outArgList.emplace_back("/incremental");

		if (m_state.toolchain.versionMajorMinor() >= 1600)
		{
			outArgList.emplace_back(getPathCommand("/ilk:", fmt::format("{}.ilk", outputFileBase)));
		}
	}
	else
	{
		outArgList.emplace_back("/incremental:NO");

		if (enableProfiling)
		{
			outArgList.emplace_back("/fixed:NO");
		}
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addDebug(StringList& outArgList, const std::string& outputFileBase) const
{
	const bool enableProfiling = m_state.configuration.enableProfiling();
	if (m_state.configuration.debugSymbols() || enableProfiling)
	{
		if (enableProfiling)
		{
			outArgList.emplace_back("/debug:FULL");
			outArgList.emplace_back("/debugtype:cv,fixup");
		}
		else
		{
			outArgList.emplace_back("/debug");
		}
		outArgList.emplace_back(getPathCommand("/pdb:", fmt::format("{}.pdb", outputFileBase)));
		outArgList.emplace_back(getPathCommand("/pdbstripped:", fmt::format("{}.stripped.pdb", outputFileBase)));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addCgThreads(StringList& outArgList) const
{
	uint maxJobs = m_state.info.maxJobs();
	if (maxJobs > 4)
	{
		maxJobs = std::min<uint>(maxJobs, 8);
		outArgList.emplace_back(fmt::format("/cgthreads:{}", maxJobs));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addDynamicBase(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/dynamicbase");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addCompatibleWithDataExecutionPrevention(StringList& outArgList) const
{
	if (!m_state.configuration.debugSymbols())
	{
		List::addIfDoesNotExist(outArgList, "/nxcompat");
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addMachine(StringList& outArgList) const
{
	auto machine = m_msvcAdapter.getMachineArchitecture();
	if (!machine.empty())
	{
		outArgList.emplace_back(fmt::format("/machine:{}", machine));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeCodeGeneration(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_state.configuration.interproceduralOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		outArgList.emplace_back("/ltcg:INCREMENTAL");
		outArgList.emplace_back(fmt::format("/ltcgout:{}.iobj", outputFileBase));

		// outArgList.emplace_back(fmt::format("/pgd:{}.pgd", outputFileBase));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addVerbosity(StringList& outArgList) const
{
	outArgList.emplace_back("/verbose:UNUSEDLIBS");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addWarningsTreatedAsErrors(StringList& outArgList) const
{
	if (m_project.treatWarningsAsErrors())
		outArgList.emplace_back("/WX");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addUnsortedOptions(StringList& outArgList) const
{
	// TODO
	// outArgList.emplace_back("/VERSION:0.0");

	UNUSED(outArgList);
}
}
