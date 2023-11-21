/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

#include "BuildEnvironment/IBuildEnvironment.hpp"
#include "Cache/WorkspaceCache.hpp"
#include "Compile/Linker/ILinker.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
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
CommandAdapterMSVC::CommandAdapterMSVC(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getPlatformToolset(const BuildState& inState)
{
	const auto majorVersion = inState.toolchain.versionMajorMinor();
	if (majorVersion >= 1700)
		return "143"; // VS 2022

	if (majorVersion >= 1600)
		return "142"; // VS 2019

	return "141"; // VS 2017
}

/*****************************************************************************/
MSVCWarningLevel CommandAdapterMSVC::getWarningLevel() const
{
	const auto& warnings = m_project.warnings();

	switch (m_project.warningsPreset())
	{
		case ProjectWarningPresets::Minimal:
			return MSVCWarningLevel::Level1;

		case ProjectWarningPresets::Extra:
			return MSVCWarningLevel::Level2;

		case ProjectWarningPresets::Pedantic:
			return MSVCWarningLevel::Level3;

		case ProjectWarningPresets::Strict:
		case ProjectWarningPresets::StrictPedantic:
			return MSVCWarningLevel::Level4;

		case ProjectWarningPresets::VeryStrict:
			// return MSVCWarningLevel::LevelAll; // Note: Lots of messy compiler level warnings that break your build!
			return MSVCWarningLevel::Level4;

		case ProjectWarningPresets::None:
			break;

		default: {
			StringList veryStrict{
				"noexcept",
				"undef",
				"conversion",
				"cast-qual",
				"float-equal",
				"inline",
				"old-style-cast",
				"strict-null-sentinel",
				"overloaded-virtual",
				"sign-conversion",
				"sign-promo",
			};

			for (auto& w : warnings)
			{
				if (!String::equals(veryStrict, w))
					continue;

				// return MSVCWarningLevel::LevelAll;
				return MSVCWarningLevel::Level4;
			}

			StringList strictPedantic{
				"unused",
				"cast-align",
				"double-promotion",
				"format=2",
				"missing-declarations",
				"missing-include-dirs",
				"non-virtual-dtor",
				"redundant-decls",
				"unreachable-code",
				"shadow",
			};
			for (auto& w : warnings)
			{
				if (!String::equals(strictPedantic, w))
					continue;

				return MSVCWarningLevel::Level4;
			}

			if (List::contains<std::string>(warnings, "pedantic"))
			{
				return MSVCWarningLevel::Level3;
			}
			else if (List::contains<std::string>(warnings, "extra"))
			{
				return MSVCWarningLevel::Level2;
			}
			else if (List::contains<std::string>(warnings, "all"))
			{
				return MSVCWarningLevel::Level1;
			}

			break;
		}
	}

	return MSVCWarningLevel::None;
}

/*****************************************************************************/
WindowsRuntimeLibraryType CommandAdapterMSVC::getRuntimeLibraryType() const
{
	if (m_project.staticRuntimeLibrary())
	{
		// Note: This will generate a larger binary!
		//
		if (m_state.configuration.debugSymbols())
			return WindowsRuntimeLibraryType::MultiThreadedDebug;
		else
			return WindowsRuntimeLibraryType::MultiThreaded;
	}
	else
	{
		if (m_state.configuration.debugSymbols())
			return WindowsRuntimeLibraryType::MultiThreadedDebugDLL;
		else
			return WindowsRuntimeLibraryType::MultiThreadedDLL;
	}
}

/*****************************************************************************/
WindowsCallingConvention CommandAdapterMSVC::getCallingConvention() const
{
	// This is the default calling convention for all functions,
	//   except C++ member functions (which use __thiscall)
	// NOTE: This really shouldn't change. Others can be explicitly defined
	return WindowsCallingConvention::Cdecl;
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getPlatformToolset() const
{
	return getPlatformToolset(m_state);
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsFastMath() const
{
	return m_project.fastMath();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsFunctionLevelLinking() const
{
	return !m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsGenerateIntrinsicFunctions() const
{
	return !m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsSDLCheck() const
{
	return true;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsConformanceMode() const
{
	return m_versionMajorMinor >= 1910; // VS 2017+
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsEditAndContinue() const
{
	const auto arch = m_state.info.targetArchitecture();

	return m_state.configuration.debugSymbols()
		&& !m_state.configuration.enableSanitizers()
		&& !m_state.configuration.enableProfiling()
		&& (arch == Arch::Cpu::X64 || arch == Arch::Cpu::X86);
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsJustMyCodeDebugging() const
{
	return m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsAddressSanitizer() const
{
	return m_versionMajorMinor >= 1928 && m_state.configuration.sanitizeAddress();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsWholeProgramOptimization() const
{
	return m_state.configuration.interproceduralOptimization();
}

bool CommandAdapterMSVC::supportsLinkTimeCodeGeneration() const
{
	return m_state.configuration.interproceduralOptimization();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsBufferSecurityCheck() const
{
	return true;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsRunTimeErrorChecks() const
{
	return m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsExceptions() const
{
	return m_project.exceptions();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsRunTimeTypeInformation() const
{
	return !disableRunTimeTypeInformation();
}

bool CommandAdapterMSVC::disableRunTimeTypeInformation() const
{
	return !m_project.runtimeTypeInformation() || !supportsExceptions();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsTreatWChartAsBuiltInType() const
{
	return true;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsForceConformanceInForLoopScope() const
{
	return true;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsRemoveUnreferencedCodeData() const
{
	return true;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsExternalWarnings() const
{
	return m_versionMajorMinor >= 1913; // added in 15.6
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsMultiProcessorCompilation() const
{
	return m_state.info.maxJobs() > 1;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsIncrementalLinking() const
{
	const auto& config = m_state.configuration;
	return config.debugSymbols() && !config.enableSanitizers() && !config.enableProfiling();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsCOMDATFolding() const
{
	return !m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsOptimizeReferences() const
{
	return m_state.configuration.enableProfiling() || !m_state.configuration.debugSymbols();
}

/*****************************************************************************/
std::optional<bool> CommandAdapterMSVC::supportsLongBranchRedirects() const
{
	const auto arch = m_state.info.targetArchitecture();
	if (arch == Arch::Cpu::ARM || arch == Arch::Cpu::ARMHF || arch == Arch::Cpu::ARM64)
	{
		return !m_state.configuration.debugSymbols();
	}

	return std::nullopt;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsProfiling() const
{
	return m_state.configuration.enableProfiling();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsDataExecutionPrevention() const
{
	return !m_state.configuration.debugSymbols();
}

/*****************************************************************************/
bool CommandAdapterMSVC::suportsILKGeneration() const
{
	return supportsIncrementalLinking() && m_state.toolchain.versionMajorMinor() >= 1600;
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsFixedBaseAddress() const
{
	return disableFixedBaseAddress();
}

bool CommandAdapterMSVC::disableFixedBaseAddress() const
{
	return m_project.isSharedLibrary() || (!supportsIncrementalLinking() && m_state.configuration.enableProfiling());
}

/*****************************************************************************/
bool CommandAdapterMSVC::enableDebugging() const
{
	return m_state.configuration.debugSymbols() || m_state.configuration.enableProfiling();
}

/*****************************************************************************/
bool CommandAdapterMSVC::supportsRandomizedBaseAddress() const
{
	return true;
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getLanguageStandardCpp() const
{
	// 2015 Update 3 or later (/std flag doesn't exist prior
	if (m_versionMajorMinor > 1900 || (m_versionMajorMinor == 1900 && m_versionPatch >= 24210))
	{
		std::string langStandard = String::toLowerCase(m_project.cppStandard());
		String::replaceAll(langStandard, "gnu++", "");
		String::replaceAll(langStandard, "c++", "");

		if (String::equals(StringList{ "20", "2a" }, langStandard))
		{
			if (m_versionMajorMinor >= 1929)
			{
				return "c++20";
			}
		}
		else if (String::equals(StringList{ "17", "1z" }, langStandard))
		{
			if (m_versionMajorMinor >= 1911)
			{
				return "c++17";
			}
		}
		else if (String::equals(StringList{ "14", "1y", "11", "0x", "03", "98" }, langStandard))
		{
			// Note: There was never "/std:c++11", "/std:c++03" or "/std:c++98"
			return "c++14";
		}

		return "c++latest";
	}

	return std::string();
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getLanguageStandardC() const
{
	// C standards conformance was added in 2019 16.8
	if (m_versionMajorMinor >= 1928)
	{
		std::string langStandard = String::toLowerCase(m_project.cStandard());
		String::replaceAll(langStandard, "gnu", "");
		String::replaceAll(langStandard, "c", "");
		if (String::equals(StringList{ "2x", "18", "17", "iso9899:2018", "iso9899:2017" }, langStandard))
		{
			return "c17";
		}
		else
		{
			return "c11";
		}
	}
	return std::string();
}

/*****************************************************************************/
char CommandAdapterMSVC::getOptimizationLevel() const
{
	OptimizationLevel level = m_state.configuration.optimizationLevel();

	if (m_state.configuration.debugSymbols()
		&& level != OptimizationLevel::Debug
		&& level != OptimizationLevel::None
		&& level != OptimizationLevel::CompilerDefault)
	{
		// force /Od (anything else would be in error)
		return 'd';
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				return '1';

			case OptimizationLevel::L2:
			case OptimizationLevel::L3:
				return '2';

			case OptimizationLevel::Size:
				return 's';

			case OptimizationLevel::Fast:
				return 't';

			case OptimizationLevel::Debug:
			case OptimizationLevel::None:
				return 'd';

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	return 0;
}

/*****************************************************************************/
char CommandAdapterMSVC::getInlineFuncExpansion() const
{
	OptimizationLevel level = m_state.configuration.optimizationLevel();

	// inline optimization flags
	//   /Ob0 - Debug
	//   /Ob1 - MinSizeRel, RelWithDebInfo
	//   /Ob2 - Release
	//   /Ob3 - If available, RelHighOpt, or with "fast"

	if (m_state.configuration.debugSymbols()
		&& level != OptimizationLevel::Debug
		&& level != OptimizationLevel::None
		&& level != OptimizationLevel::CompilerDefault)
	{
		// force /Ob0 (anything else would be in error)
		return '0';
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				if (m_state.configuration.debugSymbols())
					return '1';
				else
					return '2';

			case OptimizationLevel::L2:
				if (m_state.configuration.debugSymbols())
					return '1';
				else
					return '2';

			case OptimizationLevel::L3:
				if (m_versionMajorMinor >= 1920) // VS 2019+
					return '3';
				else
					return '2';

			case OptimizationLevel::Size:
				return '1';

			case OptimizationLevel::Fast:
				if (m_versionMajorMinor >= 1920) // VS 2019+
					return '3';
				else
					return '2';

			case OptimizationLevel::Debug:
			case OptimizationLevel::None:
				return '0';

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	return 0;
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getSubSystem() const
{
	const SourceKind kind = m_project.kind();

	// TODO: Support for /driver:WDM (NativeWDM or something)
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (kind == SourceKind::Executable)
	{
		const WindowsSubSystem subSystem = m_project.windowsSubSystem();
		switch (subSystem)
		{
			case WindowsSubSystem::Windows:
				return "windows";
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
			case WindowsSubSystem::BootApplication:
				return "BOOT_APPLICATION";

			default: break;
		}

		return "console";
	}

	return std::string();
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getEntryPoint() const
{
	const SourceKind kind = m_project.kind();
	const WindowsEntryPoint entryPoint = m_project.windowsEntryPoint();

	if (kind == SourceKind::Executable)
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
	else if (kind == SourceKind::SharedLibrary)
	{
		if (entryPoint == WindowsEntryPoint::DllMain)
		{
			return "_DllMainCRTStartup";
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string CommandAdapterMSVC::getMachineArchitecture() const
{
	// TODO: EBC?, ARM64EC
	//   Visual Studio has a list of these in "Configuration properties > Librarian > General"

	const auto arch = m_state.info.targetArchitecture();
	switch (arch)
	{
		case Arch::Cpu::X64:
			return "X64";

		case Arch::Cpu::X86:
			return "X86";

		case Arch::Cpu::ARM:
		case Arch::Cpu::ARMHF:
			return "ARM";

		case Arch::Cpu::ARM64:
			return "ARM64";

		default:
			break;
	}

	return std::string();
}

/*****************************************************************************/
StringList CommandAdapterMSVC::getIncludeDirectories() const
{
	StringList ret;

	const auto& includeDirs = m_project.includeDirs();
	for (const auto& dir : includeDirs)
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();
		ret.emplace_back(std::move(outDir));
	}

	if (m_project.usesPrecompiledHeader())
	{
		auto outDir = String::getPathFolder(m_project.precompiledHeader());
		List::addIfDoesNotExist(ret, std::move(outDir));
	}

	return ret;
}

/*****************************************************************************/
StringList CommandAdapterMSVC::getAdditionalCompilerOptions(const bool inCharsetFlags) const
{
	StringList ret = m_project.compileOptions();

	if (inCharsetFlags)
	{
		List::addIfDoesNotExist(ret, fmt::format("/source-charset:{}", m_project.inputCharset()));
		List::addIfDoesNotExist(ret, fmt::format("/execution-charset:{}", m_project.executionCharset()));
		List::addIfDoesNotExist(ret, "/validate-charset");
	}
	List::addIfDoesNotExist(ret, "/FS"); // Force Separate Program Database Writes

	if (m_project.cppCoroutines())
	{
		if (m_versionMajorMinor >= 1929)
			List::addIfDoesNotExist(ret, "/await:strict");
		else
			List::addIfDoesNotExist(ret, "/await");
	}

	// Note: in MSVC, one can combine these (annoyingly)
	//	Might be desireable to add:
	//    /Oy (suppresses the creation of frame pointers on the call stack for quicker function calls.)

	return ret;
}

/*****************************************************************************/
StringList CommandAdapterMSVC::getAdditionalLinkerOptions() const
{
	StringList ret = m_project.linkerOptions();

	auto lbr = supportsLongBranchRedirects();
	if (lbr.has_value())
	{
		if (*lbr)
			List::addIfDoesNotExist(ret, "/opt:LBR");
		else
			List::addIfDoesNotExist(ret, "/opt:NOLBR");
	}

	if (enableDebugging() && supportsProfiling())
	{
		List::addIfDoesNotExist(ret, "/debugtype:cv,fixup");
	}

	// Code Generation Threads
	/*u32 maxJobs = m_state.info.maxJobs();
	if (maxJobs > 4)
	{
		maxJobs = std::min<u32>(maxJobs, 8);
		List::addIfDoesNotExist(ret, fmt::format("/cgthreads:{}", maxJobs));
	}*/

	// Verbosity
	// TODO: confirm this actually works as intended
	// List::addIfDoesNotExist(ret, "/verbose:UNUSEDLIBS");

	return ret;
}

/*****************************************************************************/
StringList CommandAdapterMSVC::getLibDirectories() const
{
	StringList ret;

	for (const auto& dir : m_project.libDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		ret.emplace_back(std::move(outDir));
	}

	List::addIfDoesNotExist(ret, m_state.paths.buildOutputDir());

	return ret;
}

/*****************************************************************************/
StringList CommandAdapterMSVC::getLinks(const bool inIncludeCore) const
{
	StringList ret;

	// const bool hasStaticLinks = m_project.staticLinks().size() > 0;
	// const bool hasDynamicLinks = m_project.links().size() > 0;

	auto dll = m_state.environment->getSharedLibraryExtension();
	auto lib = m_state.environment->getStaticLibraryExtension();

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
					if (String::endsWith(dll, outputFile))
					{
						String::replaceAll(outputFile, dll, lib);
						List::addIfDoesNotExist(ret, std::move(outputFile));
						found = true;
						break;
					}
				}
			}
		}

		if (!found)
		{
			if (Files::pathExists(link))
				List::addIfDoesNotExist(ret, std::move(link));
			else
				List::addIfDoesNotExist(ret, fmt::format("{}{}", link, lib));
		}
	}

	if (inIncludeCore)
	{
		auto coreLinks = ILinker::getWin32CoreLibraryLinks(m_state, m_project);
		for (const auto& link : coreLinks)
		{
			List::addIfDoesNotExist(ret, fmt::format("{}{}", link, lib));
		}
	}

	return ret;
}

/*****************************************************************************/
bool CommandAdapterMSVC::createPrecompiledHeaderSource(const std::string& inSourcePath, const std::string& inPchPath)
{
	const auto& cxxExt = m_state.paths.cxxExtension();
	if (cxxExt.empty())
		return false;

	if (m_project.usesPrecompiledHeader())
	{
		bool buildHashChanged = m_state.cache.file().buildHashChanged();
		const auto& pch = m_project.precompiledHeader();

		auto ext = m_state.environment->getPrecompiledHeaderExtension();

		m_pchSource = fmt::format("{}{}.{}", inSourcePath, pch, cxxExt);
		m_pchTarget = fmt::format("{}{}{}", inPchPath, pch, ext);
		m_pchMinusLocation = String::getPathFilename(pch);

		// If The previous build with this build path (matching target triples) has an intermediate PCH file, remove it
		if (buildHashChanged)
		{
			auto oldPch = fmt::format("{}/{}", m_state.paths.objDir(), pch);
			if (Files::pathExists(oldPch))
				Files::remove(oldPch);
		}

		if (!Files::pathExists(m_pchSource))
		{
			if (!Files::createFileWithContents(m_pchSource, fmt::format("// Generated by Chalet\n\n#include \"{}\"", m_pchMinusLocation)))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
const std::string& CommandAdapterMSVC::pchSource() const noexcept
{
	return m_pchSource;
}

const std::string& CommandAdapterMSVC::pchTarget() const noexcept
{
	return m_pchTarget;
}

const std::string& CommandAdapterMSVC::pchMinusLocation() const noexcept
{
	return m_pchMinusLocation;
}
}
