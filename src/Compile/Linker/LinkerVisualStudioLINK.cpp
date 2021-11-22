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
	ILinker(inState, inProject)
{
}

/*****************************************************************************/
bool LinkerVisualStudioLINK::initialize()
{
	return true;
}

/*****************************************************************************/
StringList LinkerVisualStudioLINK::getLinkExclusions() const
{
	return { "stdc++fs" };
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
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	if (hasStaticLinks)
	{
		for (auto& target : m_state.targets)
		{
			if (target->isSources())
			{
				auto& project = static_cast<const SourceTarget&>(*target);
				if (List::contains(m_project.projectStaticLinks(), project.name()))
				{
					List::addIfDoesNotExist(outArgList, project.outputFile());
				}
			}
		}
	}

	if (hasDynamicLinks)
	{
		auto excludes = getLinkExclusions();

		for (auto& link : m_project.links())
		{
			if (List::contains(excludes, link))
				continue;

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
	}

	// TODO: Dynamic way of determining this list
	//   would they differ between console app & windows app?
	//   or target architecture?
	for (const char* link : {
			 "DbgHelp",
			 "kernel32",
			 "user32",
			 "gdi32",
			 "winspool",
			 "comdlg32",
			 "advapi32",
			 "shell32",
			 "ole32",
			 "oleaut32",
			 "uuid",
			 //  "odbc32",
			 //  "odbccp32",
		 })
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
void LinkerVisualStudioLINK::addProfileInformationLinkerOption(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/genprofile");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addSubSystem(StringList& outArgList) const
{
	const ProjectKind kind = m_project.kind();

	// TODO: Support for /driver:WDM (NativeWDM or something)
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (kind == ProjectKind::Executable)
	{
		const auto subSystem = LinkerVisualStudioLINK::getMsvcCompatibleSubSystem(m_project);
		List::addIfDoesNotExist(outArgList, fmt::format("/subsystem:{}", subSystem));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addEntryPoint(StringList& outArgList) const
{
	const auto entryPoint = LinkerVisualStudioLINK::getMsvcCompatibleEntryPoint(m_project);
	if (!entryPoint.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/entry:{}", entryPoint));
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeOptimizations(StringList& outArgList) const
{
	// if (m_state.configuration.linkTimeOptimization())
	{
		const auto arch = m_state.info.targetArchitecture();
		bool isArm = arch == Arch::Cpu::ARM || arch == Arch::Cpu::ARM64;

		// Note: These are also tied to /incremental (implied with /debug)
		if (m_state.configuration.debugSymbols())
		{
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
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addIncremental(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_state.configuration.debugSymbols())
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
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addDebug(StringList& outArgList, const std::string& outputFileBase) const
{
	if (m_state.configuration.debugSymbols())
	{
		outArgList.emplace_back("/debug");
		outArgList.emplace_back(getPathCommand("/pdb:", fmt::format("{}.pdb", outputFileBase)));
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
	// TODO: /MACHINE - target platform arch
	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			List::addIfDoesNotExist(outArgList, "/machine:X64");
			break;

		case Arch::Cpu::X86:
			List::addIfDoesNotExist(outArgList, "/machine:X86");
			break;

		case Arch::Cpu::ARM:
			List::addIfDoesNotExist(outArgList, "/machine:ARM");
			break;

		case Arch::Cpu::ARM64:
		default:
			// ??
			break;
	}
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLinkTimeCodeGeneration(StringList& outArgList, const std::string& outputFileBase) const
{
	/*if (m_state.configuration.linkTimeOptimization() && !m_state.info.dumpAssembly())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		outArgList.emplace_back("/ltcg:INCREMENTAL");
		outArgList.emplace_back(fmt::format("/ltcgout:{}.iobj", outputFileBase));
	}*/
	UNUSED(outArgList, outputFileBase);
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addVerbosity(StringList& outArgList) const
{
	outArgList.emplace_back("/verbose:UNUSEDLIBS");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addWarningsTreatedAsErrors(StringList& outArgList) const
{
	if (m_project.warningsTreatedAsErrors())
		outArgList.emplace_back("/WX");
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addUnsortedOptions(StringList& outArgList) const
{

	// if (m_state.configuration.linkTimeOptimization())
	{
		// outArgList.emplace_back(fmt::format("/pgd:{}.pgd", outputFileBase));
	}

	// TODO
	// outArgList.emplace_back("/VERSION:0.0");

	UNUSED(outArgList);
}

/*****************************************************************************/
std::string LinkerVisualStudioLINK::getMsvcCompatibleSubSystem(const SourceTarget& inProject)
{
	const WindowsSubSystem subSystem = inProject.windowsSubSystem();
	switch (subSystem)
	{
		case WindowsSubSystem::Windows:
			return "windows";
		case WindowsSubSystem::BootApplication:
			return "BOOT_APPLICATION";
		case WindowsSubSystem::Native:
			return "native";
		case WindowsSubSystem::Posix:
			return "posix";
		case WindowsSubSystem::EfiApplication:
			return "EFI_APPLICATION";
		case WindowsSubSystem::EfiBootServiceDriver:
			return "EFI_BOOT_SERVICE_DRIVER";
		case WindowsSubSystem::EfiRom:
			return "EFI_ROM";
		case WindowsSubSystem::EfiRuntimeDriver:
			return "EFI_RUNTIME_DRIVER";

		default: break;
	}

	return "console";
}

/*****************************************************************************/
std::string LinkerVisualStudioLINK::getMsvcCompatibleEntryPoint(const SourceTarget& inProject)
{
	const ProjectKind kind = inProject.kind();
	const WindowsEntryPoint entryPoint = inProject.windowsEntryPoint();

	if (kind == ProjectKind::Executable)
	{
		switch (entryPoint)
		{
			case WindowsEntryPoint::MainUnicode:
				return "wmainCRTStartup";
			case WindowsEntryPoint::WinMain:
				return "WinMainCRTStartup";
			case WindowsEntryPoint::WinMainUnicode:
				return "wWinMainCRTStartup";
			default: break;
		}

		return "mainCRTStartup";
	}
	else if (kind == ProjectKind::SharedLibrary)
	{
		if (entryPoint == WindowsEntryPoint::DllMain)
		{
			return "_DllMainCRTStartup";
		}
	}

	return std::string();
}

}
