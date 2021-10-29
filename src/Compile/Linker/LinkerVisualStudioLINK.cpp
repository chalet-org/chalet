/*
	 Distributed under the OSI-approved BSD 3-Clause License.
	 See accompanying file LICENSE.txt for details.
 */

#include "Compile/Linker/LinkerVisualStudioLINK.hpp"

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

	addArchitecture(ret, std::string());
	addSubSystem(ret);
	addEntryPoint(ret);
	addCgThreads(ret);
	addLinkerOptions(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.emplace_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.emplace_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.emplace_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.emplace_back("/debug");
		ret.emplace_back("/INCREMENTAL");

		ret.emplace_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.emplace_back("/release");
		ret.emplace_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.emplace_back("/version:0.0");

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

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

	addArchitecture(ret, std::string());
	addSubSystem(ret);
	addEntryPoint(ret);
	addCgThreads(ret);
	addLinkerOptions(ret);
	addLibDirs(ret);

	const bool debugSymbols = m_state.configuration.debugSymbols();
	if (m_state.configuration.linkTimeOptimization())
	{
		// combines w/ /GL - I think this is basically part of MS's link-time optimization
		// ret.emplace_back("/LTCG");

		// Note: These are also tied to /INCREMENTAL (implied with /debug)
		if (debugSymbols)
			ret.emplace_back("/opt:NOREF,NOICF,NOLBR");
		else
			ret.emplace_back("/opt:REF,ICF,LBR");

		// OPT:LBR - relates to arm binaries
	}

	if (debugSymbols)
	{
		ret.emplace_back("/debug");
		ret.emplace_back("/INCREMENTAL");

		ret.emplace_back(fmt::format("/pdb:{}.pdb", outputFileBase));
	}
	else
	{
		ret.emplace_back("/release");
		ret.emplace_back("/INCREMENTAL:NO");
	}

	// TODO /version
	ret.emplace_back("/version:0.0");

	ret.emplace_back(fmt::format("/out:{}", outputFile));

	addPrecompiledHeaderLink(ret);
	addSourceObjects(ret, sourceObjs);
	addLinks(ret);

	return ret;
}

/*****************************************************************************/
void LinkerVisualStudioLINK::addLibDirs(StringList& outArgList) const
{
	std::string option{ "/LIBPATH:" };
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
			if (target->isProject())
			{
				auto& project = static_cast<const SourceTarget&>(*target);
				if (List::contains(m_project.projectStaticLinks(), project.name()))
				{
					outArgList.push_back(project.outputFile());
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
				if (target->isProject())
				{
					auto& project = static_cast<const SourceTarget&>(*target);
					if (project.name() == link && project.isSharedLibrary())
					{
						auto outputFile = project.outputFile();
						if (String::endsWith(".dll", outputFile))
						{
							String::replaceAll(outputFile, ".dll", ".lib");
							outArgList.emplace_back(std::move(outputFile));
							found = true;
							break;
						}
					}
				}
			}

			if (!found)
			{
				outArgList.emplace_back(fmt::format("{}.lib", link));
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
			 "shell32",
			 "ole32",
			 "oleaut32",
			 "uuid",
			 "comdlg32",
			 "advapi32",
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
		outArgList.push_back(option);
	}
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
bool LinkerVisualStudioLINK::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	UNUSED(inArch);

	// TODO: /MACHINE - target platform arch
	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			outArgList.emplace_back("/machine:x64");
			break;

		case Arch::Cpu::X86:
			outArgList.emplace_back("/machine:x86");
			break;

		case Arch::Cpu::ARM:
			outArgList.emplace_back("/machine:arm");
			break;

		case Arch::Cpu::ARM64:
		default:
			// ??
			break;
	}

	return true;
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
void LinkerVisualStudioLINK::addPrecompiledHeaderLink(StringList outArgList) const
{
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		std::string pchObject = fmt::format("{}/{}.obj", objDir, pch);
		std::string pchInclude = fmt::format("{}/{}.pch", objDir, pch);

		outArgList.emplace_back(std::move(pchObject));
		outArgList.emplace_back(std::move(pchInclude));
	}
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
