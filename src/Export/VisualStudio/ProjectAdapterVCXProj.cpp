/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/ProjectAdapterVCXProj.hpp"

#include "Compile/CommandAdapter/CommandAdapterWinResource.hpp"
#include "Core/CommandLineInputs.hpp"
#include "DotEnv/DotEnvFileGenerator.hpp"
#include "Platform/Arch.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "System/Files.hpp"
#include "Utility/List.hpp"
#include "Utility/Path.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
ProjectAdapterVCXProj::ProjectAdapterVCXProj(const BuildState& inState, const SourceTarget& inProject) :
	m_state(inState),
	m_project(inProject),
	m_msvcAdapter(inState, inProject)
{
	m_versionMajorMinor = m_state.toolchain.compilerCxx(m_project.language()).versionMajorMinor;
	m_versionPatch = m_state.toolchain.compilerCxx(m_project.language()).versionPatch;
}

/*****************************************************************************/
bool ProjectAdapterVCXProj::createPrecompiledHeaderSource()
{
	std::string objDir{ "$(IntDir)" }; // note: objDir()
	auto intDir = getIntermediateDir();

	return m_msvcAdapter.createPrecompiledHeaderSource(intDir, objDir);
}

/*****************************************************************************/
bool ProjectAdapterVCXProj::createWindowsResources()
{
	CommandAdapterWinResource adapter(m_state, m_project);
	if (!adapter.createWindowsApplicationManifest())
		return false;

	if (!adapter.createWindowsApplicationIcon())
		return false;

	return true;
}

/*****************************************************************************/
bool ProjectAdapterVCXProj::usesPrecompiledHeader() const
{
	return m_project.usesPrecompiledHeader();
}

bool ProjectAdapterVCXProj::usesLibrarian() const
{
	return m_project.isStaticLibrary();
}

bool ProjectAdapterVCXProj::usesModules() const
{
	return m_project.cppModules();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBoolean(const bool inValue) const
{
	return std::string(inValue ? "true" : "false");
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBooleanIfTrue(const bool inValue) const
{
	if (inValue)
		return "true";

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBooleanIfFalse(const bool inValue) const
{
	if (!inValue)
		return "false";

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBuildDir() const
{
	// return "$(SolutionDir)$(Platform)_$(Configuration)\\";
	return Path::getWithSeparatorSuffix(Files::getCanonicalPath(m_state.paths.buildOutputDir()));
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getObjectDir() const
{
	auto buildDir = getBuildDir();
	return fmt::format("{}obj.{}/", buildDir, m_project.name());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getIntermediateDir() const
{
	return Path::getWithSeparatorSuffix(Files::getCanonicalPath(m_state.paths.intermediateDir()));
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEmbedManifest() const
{
	return getBoolean(false);
}

/*****************************************************************************/
const std::string& ProjectAdapterVCXProj::getTargetName() const noexcept
{
	return m_project.name();
}

const std::string& ProjectAdapterVCXProj::workingDirectory() const noexcept
{
	return m_state.inputs.workingDirectory();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getConfigurationType() const
{
	if (m_project.isExecutable())
		return "Application";

	if (m_project.isSharedLibrary())
		return "DynamicLibrary";

	if (m_project.isStaticLibrary())
		return "StaticLibrary";

	return "Utility";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getUseDebugLibraries() const
{
	return getBoolean(m_state.configuration.debugSymbols());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getPlatformToolset() const
{
	return fmt::format("v{}", m_msvcAdapter.getPlatformToolset());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getWholeProgramOptimization() const
{
	// LTCG - true/false
	// Profile Guided Optimization, Instrument - PGInstrument
	// Profile Guided Optimization, Optimize - PGOptimize
	// Profile Guided Optimization, Update - PGUpdate
	return getBooleanIfTrue(m_msvcAdapter.supportsWholeProgramOptimization());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getCharacterSet() const
{
	const auto& executionCharset = m_project.executionCharset();
	if (String::equals({ "UTF-8", "utf-8" }, executionCharset))
	{
		return "Unicode";
	}

	// TODO: More conversions
	return "MultiByte";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getFunctionLevelLinking() const
{
	// true - /Gy
	// false - /Gy-
	return getBooleanIfTrue(m_msvcAdapter.supportsFunctionLevelLinking());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getIntrinsicFunctions() const
{
	// true - /Oi
	return getBooleanIfTrue(m_msvcAdapter.supportsGenerateIntrinsicFunctions());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getSDLCheck() const
{
	// true - /sdl
	return getBooleanIfTrue(m_msvcAdapter.supportsSDLCheck());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getConformanceMode() const
{
	// true - /permissive-
	// false - /permissive
	return getBooleanIfTrue(m_msvcAdapter.supportsConformanceMode());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getWarningLevel() const
{
	switch (m_msvcAdapter.getWarningLevel())
	{
		case MSVCWarningLevel::Level1:
			return "Level1";
		case MSVCWarningLevel::Level2:
			return "Level2";
		case MSVCWarningLevel::Level3:
			return "Level3";
		case MSVCWarningLevel::Level4:
			return "Level4";
		case MSVCWarningLevel::LevelAll:
			return "LevelAll";

		default:
			break;
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getExternalWarningLevel() const
{
	if (m_msvcAdapter.supportsExternalWarnings())
	{
		switch (m_msvcAdapter.getWarningLevel())
		{
			case MSVCWarningLevel::Level1:
				return "Level1";
			case MSVCWarningLevel::Level2:
				return "Level2";
			case MSVCWarningLevel::Level3:
				return "Level3";
			case MSVCWarningLevel::Level4:
				return "Level4";

			default:
				break;
		}
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getPreprocessorDefinitions() const
{
	StringList list;
	for (auto& define : m_project.defines())
	{
		list.emplace_back(define);
	}

	if (!m_msvcAdapter.supportsExceptions())
		List::addIfDoesNotExist<std::string>(list, "_HAS_EXCEPTIONS=0");

	auto ret = String::join(list, ';');
	if (!ret.empty())
		ret += ';';

	ret += "%(PreprocessorDefinitions)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLanguageStandardCpp() const
{
	auto standard = m_msvcAdapter.getLanguageStandardCpp();
	if (standard.empty())
		return standard;

	String::replaceAll(standard, '+', 'p');
	return fmt::format("std{}", standard);
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLanguageStandardC() const
{
	auto standard = m_msvcAdapter.getLanguageStandardC();
	if (standard.empty())
		return standard;

	return fmt::format("std{}", standard);
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getMultiProcessorCompilation() const
{
	return getBoolean(m_msvcAdapter.supportsMultiProcessorCompilation());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getTreatWarningsAsError() const
{
	return getBooleanIfTrue(m_project.treatWarningsAsErrors());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getDiagnosticsFormat() const
{
	// Column is default. might be better here
	return "Caret";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getDebugInformationFormat() const
{
	if (m_msvcAdapter.supportsEditAndContinue())
		return "EditAndContinue"; // ZI

	return "ProgramDatabase"; // Zi
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getSupportJustMyCode() const
{
	return getBoolean(m_msvcAdapter.supportsJustMyCodeDebugging());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEnableAddressSanitizer() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsAddressSanitizer());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getOptimization() const
{
	// Disabled - /O0
	// MinSpace - /O1
	// MaxSpeed - /O2
	// Full     - /Ox
	char opt = m_msvcAdapter.getOptimizationLevel();
	switch (opt)
	{
		case '0':
			return "Disabled";
		case '1':
			return "MinSpace";
		case '2':
			return "MaxSpeed";
		case 'x':
			return "Full";
		default:
			break;
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getInlineFunctionExpansion() const
{
	// Disabled - /Ob0
	// OnlyExplicitInline - /Ob1
	// AnySuitable - /Ob2
	char opt = m_msvcAdapter.getInlineFuncExpansion();
	switch (opt)
	{
		case '0':
			return "Disabled";
		case '1':
			return "OnlyExplicitInline";
		case '2':
			return "AnySuitable";
		case '3':
			// TODO: don't think this is available yet, research
		default:
			break;
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getFavorSizeOrSpeed() const
{
	// Size    - /Os
	// Speed   - /Ot
	// Neither - ?
	char opt = m_msvcAdapter.getOptimizationLevel();
	switch (opt)
	{
		case 's':
			return "Size";
		case 't':
			return "Speed";
		default:
			// TODO: Does "Neither" actually do anything?
			break;
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getWholeProgramOptimizationCompileFlag() const
{
	// true/false - /GL
	return getBooleanIfTrue(m_msvcAdapter.supportsWholeProgramOptimization());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBufferSecurityCheck() const
{
	// true/false - /GS
	return getBooleanIfTrue(m_msvcAdapter.supportsBufferSecurityCheck()); // always true for now
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getFloatingPointModel() const
{
	// Fast - /fp:fast
	// Strict - /fp:strict
	// Precise - /fp:precise

	if (m_msvcAdapter.supportsFastMath())
		return "Fast";
	else
		return "Precise";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getBasicRuntimeChecks() const
{
	// StackFrameRuntimeCheck - /RTCs
	// UninitializedLocalUsageCheck - /RTCu
	// EnableFastChecks - (both) /RTC1
	if (m_msvcAdapter.supportsRunTimeErrorChecks())
		return "EnableFastChecks";

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getRuntimeLibrary() const
{
	auto type = m_msvcAdapter.getRuntimeLibraryType();
	switch (type)
	{
		case WindowsRuntimeLibraryType::MultiThreadedDebug:
			return "MultiThreadedDebug";
		case WindowsRuntimeLibraryType::MultiThreadedDebugDLL:
			return "MultiThreadedDebugDLL";
		case WindowsRuntimeLibraryType::MultiThreadedDLL:
			return "MultiThreadedDLL";
		case WindowsRuntimeLibraryType::MultiThreaded:
		default:
			break;
	}

	return "MultiThreaded";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getExceptionHandling() const
{
	// Sync - "Yes" /EHsc
	// ASync - "Yes with SEH Exceptions" /EHa
	// SyncCThrow - "Yes with Extern C functions" /EHs
	// false - "No"

	if (m_msvcAdapter.supportsExceptions())
		return "Sync";

	return "false";
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getRunTimeTypeInfo() const
{
	return getBoolean(m_msvcAdapter.supportsRunTimeTypeInformation());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getTreatWChartAsBuiltInType() const
{
	return getBoolean(m_msvcAdapter.supportsTreatWChartAsBuiltInType());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getForceConformanceInForLoopScope() const
{
	return getBoolean(m_msvcAdapter.supportsForceConformanceInForLoopScope());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getRemoveUnreferencedCodeData() const
{
	return getBoolean(m_msvcAdapter.supportsRemoveUnreferencedCodeData());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEnableUnitySupport() const
{
	return getBooleanIfTrue(m_project.unityBuild());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getCallingConvention() const
{
	// __cdecl - /Gd
	// __fastcall - /Gr
	// __stdcall - /Gz
	// __vectorcall - /Gv

	auto type = m_msvcAdapter.getCallingConvention();
	std::string flag;
	switch (type)
	{
		case WindowsCallingConvention::Cdecl:
			return "Cdecl";
		case WindowsCallingConvention::FastCall:
			return "FastCall";
		case WindowsCallingConvention::StdCall:
			return "StdCall";
		case WindowsCallingConvention::VectorCall:
		default:
			return "VectorCall";
	}
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getPrecompiledHeaderFile() const
{
	if (m_project.usesPrecompiledHeader())
		return Files::getCanonicalPath(m_project.precompiledHeader());

	return std::string();
}

const std::string& ProjectAdapterVCXProj::getPrecompiledHeaderMinusLocation() const noexcept
{
	return m_msvcAdapter.pchMinusLocation();
}

const std::string& ProjectAdapterVCXProj::getPrecompiledHeaderSourceFile() const noexcept
{
	return m_msvcAdapter.pchSource();
}

std::string ProjectAdapterVCXProj::getPrecompiledHeaderOutputFile() const
{
	auto file = String::getPathFilename(m_msvcAdapter.pchTarget());
	return fmt::format("$(IntDir){}", file);
}

std::string ProjectAdapterVCXProj::getPrecompiledHeaderObjectFile() const
{
	return m_state.paths.getPrecompiledHeaderObject(m_msvcAdapter.pchTarget());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getProgramDataBaseFileName() const
{
	return fmt::format("$(IntDir)vc$(PlatformToolsetVersion)-{}.pdb", m_project.name());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAssemblerOutput() const
{
	if (m_state.info.dumpAssembly())
		return "AssemblyCode";

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAssemblerListingLocation() const
{
	if (m_state.info.dumpAssembly())
	{
		auto buildDir = getBuildDir();
		return fmt::format("{}asm.{}/", buildDir, m_project.name());
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalIncludeDirectories(const bool inAddCwd) const
{
	auto buildDir = getBuildDir();
	auto list = m_msvcAdapter.getIncludeDirectories();
	for (auto& dir : list)
	{
		if (dir.size() > 2 && dir[1] != ':')
		{
			auto full = Files::getCanonicalPath(dir);
			dir = std::move(full);
		}
	}

	if (inAddCwd)
	{
		list.emplace_back(m_state.inputs.workingDirectory());
	}

	auto ret = String::join(list, ';');
	if (!ret.empty())
		ret += ';';

	ret += "%(AdditionalIncludeDirectories)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalCompilerOptions() const
{
	StringList options = List::combineRemoveDuplicates(m_project.compileOptions(), m_msvcAdapter.getAdditionalCompilerOptions(true));
	auto ret = String::join(options, ' ');
	if (!ret.empty())
		ret += ' ';

	ret += "%(AdditionalOptions)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getGenerateDebugInformation() const
{
	if (m_msvcAdapter.enableDebugging())
	{
		if (m_msvcAdapter.supportsProfiling())
			return "DebugFull";
		else
			return getBoolean(true);
	}
	else
	{
		return getBoolean(false);
	}
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getIncrementalLinkDatabaseFile() const
{
	if (m_msvcAdapter.suportsILKGeneration())
	{
		return "$(IntDir)$(TargetName).ilk";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getFixedBaseAddress() const
{
	// true - /FIXED (we don't want to explicity set this
	// false - /FIXED:no
	return getBooleanIfFalse(m_msvcAdapter.supportsFixedBaseAddress());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalLibraryDirectories() const
{
	auto buildDir = getBuildDir();
	buildDir.pop_back();

	auto list = m_msvcAdapter.getLibDirectories();
	for (auto& dir : list)
	{
		if (dir.size() > 2 && dir[1] != ':')
		{
			auto full = Files::getCanonicalPath(dir);
			dir = std::move(full);
		}
	}

	auto ret = String::join(list, ';');
	if (!ret.empty())
		ret += ';';

	ret += "%(AdditionalLibraryDirectories)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalDependencies() const
{
	auto links = m_msvcAdapter.getLinks();
	auto ret = String::join(links, ';');
	if (!ret.empty())
		ret += ';';

	ret += "$(CoreLibraryDependencies);%(AdditionalDependencies)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getTreatLinkerWarningAsErrors() const
{
	return getBoolean(m_project.treatWarningsAsErrors());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLinkIncremental() const
{
	return getBoolean(m_msvcAdapter.supportsIncrementalLinking());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEnableCOMDATFolding() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsCOMDATFolding());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getOptimizeReferences() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsOptimizeReferences());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLinkerLinkTimeCodeGeneration() const
{
	// TODO:  UseFastLinkTimeCodeGeneration

	if (m_msvcAdapter.supportsLinkTimeCodeGeneration())
	{
		return "UseFastLinkTimeCodeGeneration";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLinkTimeCodeGenerationObjectFile() const
{
	if (m_msvcAdapter.supportsLinkTimeCodeGeneration())
	{
		return "$(IntDir)$(TargetName).iobj";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getImportLibrary() const
{
	if (m_project.isSharedLibrary())
	{
		return "$(OutDir)$(TargetName).lib";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getProgramDatabaseFile() const
{
	if (m_msvcAdapter.enableDebugging())
	{
		return "$(OutDir)$(TargetName).pdb";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getStripPrivateSymbols() const
{
	if (m_msvcAdapter.supportsStrippedPdb())
	{
		return "$(OutDir)$(TargetName).stripped.pdb";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEntryPointSymbol() const
{
	return m_msvcAdapter.getEntryPoint();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getRandomizedBaseAddress() const
{
	return getBoolean(m_msvcAdapter.supportsRandomizedBaseAddress());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getDataExecutionPrevention() const
{
	return getBoolean(m_msvcAdapter.supportsDataExecutionPrevention());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getProfile() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsProfiling());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getSubSystem() const
{
	// TODO: Support for /driver:WDM (NativeWDM or something)
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (m_project.isExecutable())
	{
		auto subSystem = m_project.windowsSubSystem();
		switch (subSystem)
		{
			case WindowsSubSystem::Windows:
				return "Windows";
			case WindowsSubSystem::Native:
				return "Native";
			case WindowsSubSystem::Posix:
				return "POSIX";
			case WindowsSubSystem::EfiApplication:
				return "EFI Application";
			case WindowsSubSystem::EfiBootServiceDriver:
				return "EFI Boot Service Driver";
			case WindowsSubSystem::EfiRom:
				return "EFI ROM";
			case WindowsSubSystem::EfiRuntimeDriver:
				return "EFI Runtime";

			case WindowsSubSystem::BootApplication:
			default:
				break;
		}

		return "Console";
	}

	return std::string();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getEntryPoint() const
{
	return m_msvcAdapter.getEntryPoint();
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLinkTimeCodeGeneration() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsLinkTimeCodeGeneration());
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getTargetMachine() const
{
	auto machine = m_msvcAdapter.getMachineArchitecture();
	if (!machine.empty())
	{
		return fmt::format("Machine{}", machine);
	}

	return machine;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalLinkerOptions() const
{
	StringList options = m_msvcAdapter.getAdditionalLinkerOptions();
	auto ret = String::join(options, ' ');
	if (!ret.empty())
		ret += ' ';

	ret += "%(AdditionalOptions)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLocalDebuggerEnvironment() const
{
	auto gen = DotEnvFileGenerator::make(m_state);
	auto path = gen.getRunPaths();
	auto ret = fmt::format("Path={};%Path%", path);

	return ret;
}

}
