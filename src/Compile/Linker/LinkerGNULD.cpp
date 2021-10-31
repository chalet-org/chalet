/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerGNULD.hpp"

#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Utility/List.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerGNULD::LinkerGNULD(const BuildState& inState, const SourceTarget& inProject) :
	ILinker(inState, inProject)
{
}

/*****************************************************************************/
bool LinkerGNULD::initialize()
{
	// initializeSupportedLinks();

	return true;
}

/*****************************************************************************/
StringList LinkerGNULD::getLinkExclusions() const
{
	return {};
}

/*****************************************************************************/
bool LinkerGNULD::isLinkSupported(const std::string& inLink) const
{
	if (!m_supportedLinks.empty() && m_state.environment->isGcc())
	{
		return m_supportedLinks.find(inLink) != m_supportedLinks.end();
	}

	return true;
}

/*****************************************************************************/
StringList LinkerGNULD::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));

	ret.emplace_back("-shared");
	if (m_state.environment->isMingw())
	{
		std::string mingwLinkerOptions;
		if (m_project.windowsOutputDef())
		{
			mingwLinkerOptions = fmt::format("-Wl,--output-def={}.def", outputFileBase);
		}
		mingwLinkerOptions += fmt::format("-Wl,--out-implib={}.a", outputFileBase);
		ret.emplace_back(std::move(mingwLinkerOptions));

		ret.emplace_back("-Wl,--dll");
	}
	else
	{
		addPositionIndependentCodeOption(ret);
	}

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret, std::string());
	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addMacosFrameworkOptions(ret);

	addLibDirs(ret);

	ret.emplace_back("-o");
	ret.push_back(outputFile);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
StringList LinkerGNULD::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs, const std::string& outputFileBase)
{
	UNUSED(outputFileBase);

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));

	addLibDirs(ret);

	ret.emplace_back("-o");
	ret.push_back(outputFile);

	addRunPath(ret);
	addSourceObjects(ret, sourceObjs);

	addLinks(ret);

	addStripSymbolsOption(ret);
	addLinkerOptions(ret);
	addMacosSysRootOption(ret);
	addProfileInformationLinkerOption(ret);
	addLinkTimeOptimizationOption(ret);
	addThreadModelLinkerOption(ret);
	addArchitecture(ret, std::string());

	addLinkerScripts(ret);
	addLibStdCppLinkerOption(ret);
	addStaticCompilerLibraryOptions(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addMacosFrameworkOptions(ret);

	return ret;
}

/*****************************************************************************/
void LinkerGNULD::addLibDirs(StringList& outArgList) const
{
	const std::string prefix{ "-L" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(prefix, dir));
	}

	outArgList.emplace_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	// must be last
	std::string localLib{ "/usr/local/lib/" };
	if (Commands::pathExists(localLib))
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, localLib));
#endif
}

/*****************************************************************************/
void LinkerGNULD::addLinks(StringList& outArgList) const
{
	const std::string prefix{ "-l" };
	const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	const bool hasDynamicLinks = m_project.links().size() > 0;

	if (hasStaticLinks)
	{
		startStaticLinkGroup(outArgList);

		for (auto& staticLink : m_project.staticLinks())
		{
			if (isLinkSupported(staticLink))
				outArgList.emplace_back(prefix + staticLink);
		}

		endStaticLinkGroup(outArgList);
		startExplicitDynamicLinkGroup(outArgList);
	}

	if (hasDynamicLinks)
	{
		auto excludes = getLinkExclusions();

		for (auto& link : m_project.links())
		{
			if (List::contains(excludes, link))
				continue;

			if (isLinkSupported(link))
				outArgList.emplace_back(prefix + link);
		}
	}
}

/*****************************************************************************/
void LinkerGNULD::addRunPath(StringList& outArgList) const
{
#if defined(CHALET_LINUX)
	outArgList.emplace_back("-Wl,-rpath,'$$ORIGIN'"); // Note: Single quotes are required!
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
void LinkerGNULD::addStripSymbolsOption(StringList& outArgList) const
{
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	if (m_state.configuration.stripSymbols())
	{
		std::string strip{ "-s" };
		// if (isFlagSupported(strip))
		outArgList.emplace_back(std::move(strip));
	}
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
void LinkerGNULD::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		// if (isFlagSupported(option))
		outArgList.push_back(option);
	}
}

/*****************************************************************************/
void LinkerGNULD::addProfileInformationLinkerOption(StringList& outArgList) const
{
	const bool enableProfiling = m_state.configuration.enableProfiling();
	if (enableProfiling && m_project.isExecutable())
	{
		outArgList.emplace_back("-Wl,--allow-multiple-definition");

		std::string profileInfo{ "-pg" };
		// if (isFlagSupported(profileInfo))
		outArgList.emplace_back(std::move(profileInfo));
	}
}

/*****************************************************************************/
void LinkerGNULD::addLinkTimeOptimizationOption(StringList& outArgList) const
{
	if (m_state.configuration.linkTimeOptimization())
	{
		std::string lto{ "-flto" };
		// if (isFlagSupported(lto))
		List::addIfDoesNotExist(outArgList, std::move(lto));
	}
}

/*****************************************************************************/
void LinkerGNULD::addThreadModelLinkerOption(StringList& outArgList) const
{
	auto threadType = m_project.threadType();
	if (!m_state.environment->isWindowsClang() && (threadType == ThreadType::Posix || threadType == ThreadType::Auto))
	{
		if (m_state.environment->isMingw() && m_project.staticLinking())
		{
			outArgList.emplace_back("-Wl,-Bstatic,--whole-archive");
			outArgList.emplace_back("-lwinpthread");
			outArgList.emplace_back("-Wl,--no-whole-archive");
		}
		else
		{
			List::addIfDoesNotExist(outArgList, "-pthread");
		}
	}
}

/*****************************************************************************/
void LinkerGNULD::addLinkerScripts(StringList& outArgList) const
{
	const auto& linkerScript = m_project.linkerScript();
	if (!linkerScript.empty())
	{
		outArgList.emplace_back("-T");
		outArgList.push_back(linkerScript);
	}
}

/*****************************************************************************/
void LinkerGNULD::addLibStdCppLinkerOption(StringList& outArgList) const
{
	// Not used in GCC
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerGNULD::addStaticCompilerLibraryOptions(StringList& outArgList) const
{
	// List::addIfDoesNotExist(outArgList, "-libstdc++");

	if (m_project.staticLinking())
	{
		auto addFlag = [&](std::string flag) {
			// if (isFlagSupported(flag))
			List::addIfDoesNotExist(outArgList, std::move(flag));
		};

		addFlag("-static-libgcc");
		addFlag("-static-libasan");
		addFlag("-static-libtsan");
		addFlag("-static-liblsan");
		addFlag("-static-libubsan");
		addFlag("-static-libstdc++");
	}
}

/*****************************************************************************/
void LinkerGNULD::addSubSystem(StringList& outArgList) const
{
	if (m_state.environment->isMingwGcc())
	{
		// MinGW rolls these together for some reason
		// -mwindows and -mconsole kind of do some magic behind the scenes, so it's hard to assume anything

		const ProjectKind kind = m_project.kind();
		const WindowsSubSystem subSystem = m_project.windowsSubSystem();
		const WindowsEntryPoint entryPoint = m_project.windowsEntryPoint();

		if (kind == ProjectKind::Executable)
		{
			if (entryPoint == WindowsEntryPoint::WinMainUnicode || entryPoint == WindowsEntryPoint::MainUnicode)
			{
				List::addIfDoesNotExist(outArgList, "-municode");
			}

			if (subSystem == WindowsSubSystem::Windows)
			{
				List::addIfDoesNotExist(outArgList, "-mwindows");
			}
			else
			{
				List::addIfDoesNotExist(outArgList, "-mconsole");
			}
		}
		else if (kind == ProjectKind::SharedLibrary)
		{
			if (entryPoint == WindowsEntryPoint::DllMain)
			{
				List::addIfDoesNotExist(outArgList, "-mdll");
			}
		}
	}
}

/*****************************************************************************/
void LinkerGNULD::addEntryPoint(StringList& outArgList) const
{
	UNUSED(outArgList);

	// MinGW: See addSubSystem
}

/*****************************************************************************/
void LinkerGNULD::startStaticLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,--copy-dt-needed-entries");
	outArgList.emplace_back("-Wl,-Bstatic");
	outArgList.emplace_back("-Wl,--start-group");
#endif
}

void LinkerGNULD::endStaticLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,--end-group");
#endif
}

/*****************************************************************************/
void LinkerGNULD::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,-Bdynamic");
#endif
}

/*****************************************************************************/
void LinkerGNULD::addCompilerSearchPaths(StringList& outArgList) const
{
	// Same as addLinks, but with -B, so far, just used in specific gcc calls
	const std::string prefix{ "-B" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(prefix, dir));
	}

	outArgList.emplace_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));

#if defined(CHALET_MACOS) || defined(CHALET_LINUX)
	// must be last
	std::string localLib{ "/usr/local/lib/" };
	if (Commands::pathExists(localLib))
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, localLib));
#endif
}

/*****************************************************************************/
void LinkerGNULD::addObjectiveCxxLink(StringList& outArgList) const
{
	UNUSED(outArgList);

	// Removed for now - seems the most concise way to use Objective-C on Linux/MinGW is via gnustep
	//   gcc `gnustep-config --objc-flags` -L/usr/GNUstep/Local/Library/Libraries -lgnustep-base foo.m -o foo

	/*if (m_project.objectiveCxx())
	{
		const std::string prefix{ "-l" };
		std::string objc = prefix + "objc";
		List::addIfDoesNotExist(outArgList, std::move(objc));
	}*/
}

/*****************************************************************************/
void LinkerGNULD::addMacosFrameworkOptions(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	// TODO: Test Homebrew LLVM/GCC with this
	{
		const std::string prefix = "-F";
		for (auto& path : m_project.libDirs())
		{
			outArgList.emplace_back(prefix + path);
		}
		for (auto& path : m_project.macosFrameworkPaths())
		{
			outArgList.emplace_back(prefix + path);
		}
		List::addIfDoesNotExist(outArgList, prefix + "/Library/Frameworks");
	}
	{
		// const std::string suffix = ".framework";
		for (auto& framework : m_project.macosFrameworks())
		{
			outArgList.emplace_back("-framework");
			outArgList.push_back(framework);
			// outArgList.emplace_back(framework + suffix);
		}
	}
#else
	UNUSED(outArgList);
#endif
}

/*****************************************************************************/
bool LinkerGNULD::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxGCC::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
bool LinkerGNULD::addMacosSysRootOption(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	return CompilerCxxAppleClang::addMacosSysRootOption(outArgList, m_state);
#else
	UNUSED(outArgList);
	return true;
#endif
}

void LinkerGNULD::addPositionIndependentCodeOption(StringList& outArgList) const
{
	if (!m_state.environment->isMingw())
	{
		std::string fpic{ "-fPIC" };
		// if (isFlagSupported(fpic))
		List::addIfDoesNotExist(outArgList, std::move(fpic));
	}
}

/*****************************************************************************/
/*void LinkerGNULD::initializeSupportedLinks()
{
	bool oldQuotePaths = m_quotePaths;
	m_quotePaths = false;

	auto isLinkSupportedByExecutable = [](const std::string& inExecutable, const std::string& inLink, const StringList& inDirectories) -> bool {
		// This will print the input if the link is not found in path
		auto file = fmt::format("lib{}.a", inLink);
		StringList cmd{ inExecutable };
		for (auto& dir : inDirectories)
		{
			cmd.push_back(dir);
		}
		cmd.emplace_back(fmt::format("-print-file-name={}", file));

		auto raw = Commands::subprocessOutput(cmd);
		// auto split = String::split(raw, '\n');

		return !String::equals(file, raw);
	};

	// TODO: Links coming from CMake projects

	StringList cmakeProjects;
	StringList projectLinks;
	for (auto& target : m_state.targets)
	{
		if (target->isProject())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
				continue;

			auto file = project.name();
			if (project.isStaticLibrary())
			{
				file += "-s";
			}
			projectLinks.emplace_back(std::move(file));
		}
		else if (target->isCMake())
		{
			// Try to guess based on the project name
			cmakeProjects.push_back(target->name());
		}
	}

	StringList libDirs;
	addCompilerSearchPaths(libDirs); // do not quote paths for this

	auto excludes = getLinkExclusions();

	const auto& exec = m_state.toolchain.compilerCxx(m_project.language()).path;

	for (auto& staticLink : m_project.staticLinks())
	{
		if (isLinkSupportedByExecutable(exec, staticLink, libDirs) || List::contains(projectLinks, staticLink) || String::contains(cmakeProjects, staticLink))
			m_supportedLinks.emplace(staticLink, true);
	}

	for (auto& link : m_project.links())
	{
		if (List::contains(excludes, link))
			continue;

		if (isLinkSupportedByExecutable(exec, link, libDirs) || List::contains(projectLinks, link) || String::contains(cmakeProjects, link))
			m_supportedLinks.emplace(link, true);
	}

	m_quotePaths = oldQuotePaths;

	m_supportedLinksInitialized = true;
}*/
}
