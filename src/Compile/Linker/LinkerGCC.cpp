/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/Linker/LinkerGCC.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Compile/CompilerCxx/CompilerCxxAppleClang.hpp"
#include "Compile/CompilerCxx/CompilerCxxGCC.hpp"
// #include "Process/Process.hpp"
#include "State/AncillaryTools.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
LinkerGCC::LinkerGCC(const BuildState& inState, const SourceTarget& inProject) :
	ILinker(inState, inProject)
{
}

/*****************************************************************************/
bool LinkerGCC::initialize()
{
	// initializeSupportedLinks();

	return true;
}

/*****************************************************************************/
void LinkerGCC::getCommandOptions(StringList& outArgList)
{
	addPositionIndependentCodeOption(outArgList);
	addStripSymbols(outArgList);
	addLinkerOptions(outArgList);
	addSystemRootOption(outArgList);
	addProfileInformation(outArgList);
	addLinkTimeOptimizations(outArgList);
	addThreadModelLinks(outArgList);
	addArchitecture(outArgList, std::string());
	addLibStdCppLinkerOption(outArgList);
	addSanitizerOptions(outArgList);
	addStaticCompilerLibraries(outArgList);
	addSubSystem(outArgList);
	addEntryPoint(outArgList);
	addAppleFrameworkOptions(outArgList);
}

/*****************************************************************************/
bool LinkerGCC::isLinkSupported(const std::string& inLink) const
{
	if (!m_supportedLinks.empty() && m_state.environment->isGcc())
	{
		return m_supportedLinks.find(inLink) != m_supportedLinks.end();
	}

	return true;
}

/*****************************************************************************/
StringList LinkerGCC::getSharedLibTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	if (!addExecutable(ret))
		return ret;

	addFuseLdOption(ret);
	addSharedOption(ret);

	if (m_state.environment->isMingw())
	{
		std::string mingwLinkerOptions;
		if (m_project.windowsOutputDef())
		{
			mingwLinkerOptions = fmt::format("-Wl,--output-def={}.def", outputFileBase());
		}
		mingwLinkerOptions += fmt::format("-Wl,--out-implib={}.a", outputFileBase());
		ret.emplace_back(std::move(mingwLinkerOptions));

		ret.emplace_back("-Wl,--dll");
	}

	addPositionIndependentCodeOption(ret);
	addStripSymbols(ret);
	addLinkerOptions(ret);
	addSystemRootOption(ret);
	addProfileInformation(ret);
	addLinkTimeOptimizations(ret);
	addThreadModelLinks(ret);
	addArchitecture(ret, std::string());
	addLibStdCppLinkerOption(ret);
	addSanitizerOptions(ret);
	addStaticCompilerLibraries(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addAppleFrameworkOptions(ret);
	addRunPath(ret);

	addLibDirs(ret);
	addSystemLibDirs(ret);

	ret.emplace_back("-o");
	ret.emplace_back(getQuotedPath(outputFile));
	addSourceObjects(ret, sourceObjs);

	addCppFilesystem(ret);
	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
StringList LinkerGCC::getExecutableTargetCommand(const std::string& outputFile, const StringList& sourceObjs)
{
	StringList ret;

	if (!addExecutable(ret))
		return ret;

	addFuseLdOption(ret);
	addExecutableOption(ret);
	addPositionIndependentCodeOption(ret);
	addStripSymbols(ret);
	addLinkerOptions(ret);
	addSystemRootOption(ret);
	addProfileInformation(ret);
	addLinkTimeOptimizations(ret);
	addThreadModelLinks(ret);
	addArchitecture(ret, std::string());
	addLibStdCppLinkerOption(ret);
	addSanitizerOptions(ret);
	addStaticCompilerLibraries(ret);
	addSubSystem(ret);
	addEntryPoint(ret);
	addAppleFrameworkOptions(ret);
	addRunPath(ret);

	addLibDirs(ret);
	addSystemLibDirs(ret);

	ret.emplace_back("-o");
	ret.emplace_back(getQuotedPath(outputFile));

	addSourceObjects(ret, sourceObjs);

	addCppFilesystem(ret);
	addLinks(ret);
	addObjectiveCxxLink(ret);

	return ret;
}

/*****************************************************************************/
bool LinkerGCC::addExecutable(StringList& outArgList) const
{
	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;
	if (executable.empty())
		return false;

	outArgList.emplace_back(getQuotedPath(executable));
	return true;
}

/*****************************************************************************/
void LinkerGCC::addLibDirs(StringList& outArgList) const
{
	const std::string prefix{ "-L" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(prefix, dir));
	}

	outArgList.emplace_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));
}

/*****************************************************************************/
void LinkerGCC::addLinks(StringList& outArgList) const
{
	const std::string prefix{ "-l" };
	const auto& staticLinks = m_project.staticLinks();
	const auto& sharedLinks = m_project.links();
	const auto& projectSharedLinks = m_project.projectSharedLinks();

	const bool isEmscripten = m_state.environment->isEmscripten();
	auto search = m_state.environment->getStaticLibraryExtension();

	if (!staticLinks.empty())
	{
		startStaticLinkGroup(outArgList);

		for (auto& link : staticLinks)
		{
			if (isLinkSupported(link))
			{
				bool resolved = false;
				if (String::endsWith(search, link))
				{
					if (Files::pathExists(link))
					{
						outArgList.emplace_back(getQuotedPath(link));
						resolved = true;
					}
					else
					{
						const auto& libDirs = m_project.libDirs();
						for (auto& dir : libDirs)
						{
							auto path = fmt::format("{}/{}", dir, link);
							if (Files::pathExists(path))
							{
								outArgList.emplace_back(getQuotedPath(path));
								resolved = true;
								break;
							}
						}
					}
				}

				if (!resolved)
					outArgList.emplace_back(prefix + link);
			}
		}

		endStaticLinkGroup(outArgList);
		startExplicitDynamicLinkGroup(outArgList);
	}

	if (!sharedLinks.empty())
	{
		for (auto& link : sharedLinks)
		{
			if (isLinkSupported(link))
			{
				bool resolved = false;
				if (String::endsWith(search, link))
				{
					if (Files::pathExists(link))
					{
						outArgList.emplace_back(getQuotedPath(link));
						resolved = true;
					}
					else
					{
						const auto& libDirs = m_project.libDirs();
						for (auto& dir : libDirs)
						{
							auto path = fmt::format("{}/{}", dir, link);
							if (Files::pathExists(path))
							{
								outArgList.emplace_back(getQuotedPath(path));
								resolved = true;
								break;
							}
						}
					}
				}

				if (!resolved)
				{
					if (isEmscripten && List::contains(projectSharedLinks, link))
						continue;

					outArgList.emplace_back(prefix + link);
				}
			}
		}
	}

	if (m_state.environment->isMingw())
	{
		auto win32Links = getWin32CoreLibraryLinks();
		for (const auto& link : win32Links)
		{
			List::addIfDoesNotExist(outArgList, fmt::format("{}{}", prefix, link));
		}
	}
}

/*****************************************************************************/
void LinkerGCC::addRunPath(StringList& outArgList) const
{
	if (m_project.isExecutable())
	{
#if defined(CHALET_LINUX)
		if (m_state.toolchain.strategy() == StrategyType::Native)
			outArgList.emplace_back("-Wl,-rpath=$ORIGIN"); // Note: Single quotes are required!
		else
			outArgList.emplace_back("-Wl,-rpath,'$$ORIGIN'"); // Note: Single quotes are required!
#elif defined(CHALET_MACOS)
		outArgList.emplace_back(fmt::format("-Wl,-install_name,@rpath/{}", String::getPathBaseName(outputFileBase())));
		outArgList.emplace_back("-Wl,-rpath,@executable_path/.");
#else
		UNUSED(outArgList);
#endif
	}
	else if (m_project.isSharedLibrary())
	{
#if defined(CHALET_MACOS)
		outArgList.emplace_back(fmt::format("-Wl,-install_name,@rpath/{}.dylib", String::getPathBaseName(outputFileBase())));
#else
		UNUSED(outArgList);
#endif
	}
}

/*****************************************************************************/
void LinkerGCC::addStripSymbols(StringList& outArgList) const
{
#if defined(CHALET_WIN32) || defined(CHALET_LINUX)
	if (!m_state.configuration.debugSymbols())
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
void LinkerGCC::addLinkerOptions(StringList& outArgList) const
{
	for (auto& option : m_project.linkerOptions())
	{
		// if (isFlagSupported(option))
		outArgList.push_back(option);
	}
}

/*****************************************************************************/
void LinkerGCC::addProfileInformation(StringList& outArgList) const
{
	// TODO: isExecutable or !isSharedLibrary
	if (m_state.configuration.enableProfiling() && m_project.isExecutable())
	{
		// Note: This was added at some point, but don't remember why...
		// outArgList.emplace_back("-Wl,--allow-multiple-definition");

		std::string profileInfo{ "-pg" };
		// if (isFlagSupported(profileInfo))
		outArgList.emplace_back(std::move(profileInfo));
	}
}

/*****************************************************************************/
void LinkerGCC::addLinkTimeOptimizations(StringList& outArgList) const
{
	if (m_state.configuration.interproceduralOptimization())
	{
		std::string lto{ "-flto" };
		// if (isFlagSupported(lto))
		List::addIfDoesNotExist(outArgList, std::move(lto));
	}
}

/*****************************************************************************/
void LinkerGCC::addThreadModelLinks(StringList& outArgList) const
{
	if (m_project.threads()
		&& !m_state.environment->isWindowsClang()
		&& !m_state.environment->isMingwClang()
		&& !m_state.environment->isEmbeddedTarget())
	{
		if (m_state.environment->isMingw() && m_project.staticRuntimeLibrary())
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
void LinkerGCC::addLibStdCppLinkerOption(StringList& outArgList) const
{
	// Not used in GCC
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerGCC::addSanitizerOptions(StringList& outArgList) const
{
	if (m_state.configuration.enableSanitizers())
	{
		CompilerCxxGCC::addSanitizerOptions(outArgList, m_state);
	}
}

/*****************************************************************************/
void LinkerGCC::addStaticCompilerLibraries(StringList& outArgList) const
{
	// List::addIfDoesNotExist(outArgList, "-libstdc++");

	if (m_project.staticRuntimeLibrary())
	{
		auto addFlag = [&outArgList](std::string flag) {
			// if (isFlagSupported(flag))
			List::addIfDoesNotExist(outArgList, std::move(flag));
		};

		if (m_state.configuration.sanitizeAddress())
			addFlag("-static-libasan");

		// Not yet
		// if (m_state.configuration.sanitizeHardwareAddress())
		// 	addFlag("-static-libhwasan");

		if (m_state.configuration.sanitizeThread())
			addFlag("-static-libtsan");

		if (m_state.configuration.sanitizeLeaks())
			addFlag("-static-liblsan");

		if (m_state.configuration.sanitizeUndefinedBehavior())
			addFlag("-static-libubsan");

		if (m_project.language() == CodeLanguage::CPlusPlus)
			addFlag("-static-libstdc++");

		addFlag("-static-libgcc");
	}
}

/*****************************************************************************/
void LinkerGCC::addSubSystem(StringList& outArgList) const
{
	if (m_state.environment->isMingwGcc())
	{
		// MinGW rolls these together for some reason
		// -mwindows and -mconsole kind of do some magic behind the scenes, so it's hard to assume anything

		const SourceKind kind = m_project.kind();
		const WindowsSubSystem subSystem = m_project.windowsSubSystem();
		const WindowsEntryPoint entryPoint = m_project.windowsEntryPoint();

		if (kind == SourceKind::Executable)
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
		else if (kind == SourceKind::SharedLibrary)
		{
			if (entryPoint == WindowsEntryPoint::DllMain)
			{
				List::addIfDoesNotExist(outArgList, "-mdll");
			}
		}
	}
}

/*****************************************************************************/
void LinkerGCC::addEntryPoint(StringList& outArgList) const
{
	UNUSED(outArgList);

	// MinGW: See addSubSystem
}

/*****************************************************************************/
void LinkerGCC::addFuseLdOption(StringList& outArgList) const
{
	auto& linker = m_state.toolchain.linker();
	if (linker.empty())
		return;

	auto exec = String::toLowerCase(String::getPathFilename(linker));
	if (String::endsWith(".exe", exec))
		exec = String::getPathFolderBaseName(exec);

	if (String::startsWith("ld.", exec))
		exec = String::getPathSuffix(exec);

	if (String::equals({ "bfd", "gold", "lld", "mold" }, exec))
	{
		List::addIfDoesNotExist(outArgList, fmt::format("-fuse-ld={}", exec));
	}
}

/*****************************************************************************/
void LinkerGCC::addCppFilesystem(StringList& outArgList) const
{
	if (m_project.cppFilesystem() && m_versionMajorMinor >= 701 && m_versionMajorMinor < 901)
	{
		std::string option{ "-lstdc++fs" };
		// if (isFlagSupported(option))
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

/*****************************************************************************/
void LinkerGCC::startStaticLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,--copy-dt-needed-entries");
	outArgList.emplace_back("-Wl,-Bstatic");
	outArgList.emplace_back("-Wl,--start-group");
#endif
}

void LinkerGCC::endStaticLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,--end-group");
#endif
}

/*****************************************************************************/
void LinkerGCC::startExplicitDynamicLinkGroup(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	UNUSED(outArgList);
#else
	outArgList.emplace_back("-Wl,-Bdynamic");
#endif
}

/*****************************************************************************/
void LinkerGCC::addCompilerSearchPaths(StringList& outArgList) const
{
	// Same as addLinks, but with -B, so far, just used in specific gcc calls
	const std::string prefix{ "-B" };
	for (const auto& dir : m_project.libDirs())
	{
		outArgList.emplace_back(getPathCommand(prefix, dir));
	}

	outArgList.emplace_back(getPathCommand(prefix, m_state.paths.buildOutputDir()));
}

/*****************************************************************************/
void LinkerGCC::addObjectiveCxxLink(StringList& outArgList) const
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
void LinkerGCC::addAppleFrameworkOptions(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	{
		const std::string prefix = "-F";
		for (auto& path : m_project.libDirs())
		{
			outArgList.emplace_back(getPathCommand(prefix, path));
		}
		for (auto& path : m_project.appleFrameworkPaths())
		{
			outArgList.emplace_back(getPathCommand(prefix, path));
		}
		List::addIfDoesNotExist(outArgList, getPathCommand(prefix, "/Library/Frameworks"));
	}
	{
		// const std::string suffix = Files::getPlatformFrameworkExtension();
		for (auto& framework : m_project.appleFrameworks())
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
bool LinkerGCC::addArchitecture(StringList& outArgList, const std::string& inArch) const
{
	return CompilerCxxGCC::addArchitectureToCommand(outArgList, inArch, m_state);
}

/*****************************************************************************/
bool LinkerGCC::addSystemRootOption(StringList& outArgList) const
{
#if defined(CHALET_MACOS)
	return CompilerCxxAppleClang::addSystemRootOption(outArgList, m_state, this->quotedPaths());
#else
	if (m_state.environment->isEmbeddedTarget())
	{
		outArgList.emplace_back("--specs=nosys.specs");
	}

	UNUSED(outArgList);
	return true;
#endif
}

/*****************************************************************************/
bool LinkerGCC::addSystemLibDirs(StringList& outArgList) const
{
#if defined(CHALET_LINUX)
	const auto& systemIncludes = m_state.environment->targetSystemPaths();
	const auto& sysroot = m_state.environment->sysroot();
	if (!systemIncludes.empty() && !sysroot.empty())
	{
		const std::string prefix{ "-L" };
		outArgList.emplace_back(getPathCommand(prefix, sysroot));
	}

	return true;
#else
	UNUSED(outArgList);
	return true;
#endif
}

/*****************************************************************************/
void LinkerGCC::addSharedOption(StringList& outArgList) const
{
	outArgList.emplace_back("-shared");
}

void LinkerGCC::addExecutableOption(StringList& outArgList) const
{
	UNUSED(outArgList);
}

/*****************************************************************************/
void LinkerGCC::addPositionIndependentCodeOption(StringList& outArgList) const
{
	if (!m_state.environment->isWindowsTarget())
	{
		if (m_project.positionIndependentCode())
		{
			std::string option{ "-fPIC" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
		else if (m_project.positionIndependentExecutable())
		{
			std::string option{ "-fPIE" };
			// if (isFlagSupported(option))
			List::addIfDoesNotExist(outArgList, std::move(option));
		}
	}
}

/*****************************************************************************/
/*void LinkerGCC::initializeSupportedLinks()
{
	auto isLinkSupportedByExecutable = [](const std::string& inExecutable, const std::string& inLink, const StringList& inDirectories) -> bool {
		// This will print the input if the link is not found in path
		auto file = fmt::format("lib{}.a", inLink);
		StringList cmd{ inExecutable };
		for (auto& dir : inDirectories)
		{
			cmd.push_back(dir);
		}
		cmd.emplace_back(fmt::format("-print-file-name={}", file));

		auto raw = Process::runOutput(cmd);
		// auto split = String::split(raw, '\n');

		return !String::equals(file, raw);
	};

	// TODO: Links coming from CMake projects

	StringList cmakeProjects;
	StringList projectLinks;
	for (auto& target : m_state.targets)
	{
		if (target->isSources())
		{
			auto& project = static_cast<const SourceTarget&>(*target);
			if (project.isExecutable())
				continue;

			auto file = project.name();
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

	const auto& exec = m_state.toolchain.compilerCxx(m_project.language()).path;

	for (auto& staticLink : m_project.staticLinks())
	{
		if (isLinkSupportedByExecutable(exec, staticLink, libDirs) || List::contains(projectLinks, staticLink) || String::contains(cmakeProjects, staticLink))
			m_supportedLinks.emplace(staticLink, true);
	}

	for (auto& link : m_project.links())
	{
		if (isLinkSupportedByExecutable(exec, link, libDirs) || List::contains(projectLinks, link) || String::contains(cmakeProjects, link))
			m_supportedLinks.emplace(link, true);
	}

	m_supportedLinksInitialized = true;
}*/
}
