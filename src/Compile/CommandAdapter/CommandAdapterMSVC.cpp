/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CommandAdapter/CommandAdapterMSVC.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
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
			MSVCWarningLevel ret = MSVCWarningLevel::None;

			bool strictSet = false;
			for (auto& w : warnings)
			{
				if (!String::equals(veryStrict, w))
					continue;

				// ret = MSVCWarningLevel::LevelAll;
				ret = MSVCWarningLevel::Level4;
				strictSet = true;
				break;
			}

			if (!strictSet)
			{
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

					ret = MSVCWarningLevel::Level4;
					strictSet = true;
					break;
				}
			}

			if (!strictSet)
			{
				if (List::contains<std::string>(warnings, "pedantic"))
				{
					ret = MSVCWarningLevel::Level3;
				}
				else if (List::contains<std::string>(warnings, "extra"))
				{
					ret = MSVCWarningLevel::Level2;
				}
				else if (List::contains<std::string>(warnings, "all"))
				{
					ret = MSVCWarningLevel::Level1;
				}
			}

			break;

			return ret;
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
	const auto majorVersion = m_state.toolchain.versionMajorMinor();
	if (majorVersion >= 1700)
		return "v143"; // VS 2022

	if (majorVersion >= 1600)
		return "v142"; // VS 2019

	return "v141"; // VS 2017
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
	return !m_state.configuration.debugSymbols();
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

	for (const auto& dir : m_project.includeDirs())
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
StringList CommandAdapterMSVC::getAdditionalOptions(const bool inCharsetFlags) const
{
	StringList ret;

	if (inCharsetFlags)
	{
		ret.emplace_back(fmt::format("/source-charset:{}", m_project.inputCharset()));
		ret.emplace_back(fmt::format("/execution-charset:{}", m_project.executionCharset()));
		ret.emplace_back("/validate-charset");
	}
	ret.emplace_back("/FS"); // Force Separate Program Database Writes

	return ret;
}

/*****************************************************************************/
bool CommandAdapterMSVC::createPrecompiledHeaderSource(const std::string& inOutputPath)
{
	const auto& cxxExt = m_state.paths.cxxExtension();
	if (cxxExt.empty())
		return false;

	if (m_project.usesPrecompiledHeader())
	{
		const auto& pch = m_project.precompiledHeader();

		m_pchSource = fmt::format("{}/{}.{}", inOutputPath, pch, cxxExt);
		m_pchTarget = fmt::format("{}/{}.pch", inOutputPath, pch);
		m_pchMinusLocation = String::getPathFilename(pch);

		if (!Commands::pathExists(m_pchSource))
		{
			auto folder = String::getPathFolder(m_pchSource);
			if (!Commands::pathExists(folder))
			{
				if (!Commands::makeDirectory(folder))
					return false;
			}

			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", m_pchMinusLocation)))
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
