/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Export/VisualStudio/ProjectAdapterVCXProj.hpp"

#include "Core/Arch.hpp"
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
	auto outputDirectory = m_state.paths.outputDirectory();
	outputDirectory += '/';
	auto sourcePath = m_state.paths.intermediateDir(m_project);
	String::replaceAll(sourcePath, outputDirectory, "");
	sourcePath += '/';

	// auto arch = Arch::toVSArch(m_state.info.targetArchitecture());
	// const auto& config = m_state.configuration.name();
	// auto sourcePath = fmt::format("{}{}/", arch, config);
	std::string pchPath{ "$(IntDir)" };

	return m_msvcAdapter.createPrecompiledHeaderSource(sourcePath, pchPath);
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
	return m_msvcAdapter.getPlatformToolset();
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
std::string ProjectAdapterVCXProj::getPrecompiledHeaderFile(const std::string& inCwd) const
{
	if (m_project.usesPrecompiledHeader())
		return fmt::format("{}/{}", inCwd, m_project.precompiledHeader());

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
std::string ProjectAdapterVCXProj::getAdditionalIncludeDirectories(const std::string& inCwd) const
{
	auto list = m_msvcAdapter.getIncludeDirectories(true);
	for (auto& dir : list)
	{
		auto full = fmt::format("{}/{}", inCwd, dir);
		if (Commands::pathExists(full))
		{
			dir = std::move(full);
		}
	}

	auto ret = String::join(list, ';');
	if (!ret.empty())
		ret += ';';

	ret += "%(AdditionalIncludeDirectories)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getAdditionalOptions() const
{
	StringList options = List::combine(m_project.compileOptions(), m_msvcAdapter.getAdditionalOptions(true));
	auto ret = String::join(options, ' ');
	if (!ret.empty())
		ret += ' ';

	ret += "%(AdditionalOptions)";
	return ret;
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getGenerateDebugInformation() const
{
	return getBoolean(true);
}

/*****************************************************************************/
std::string ProjectAdapterVCXProj::getLinkIncremental() const
{
	return getBooleanIfTrue(m_msvcAdapter.supportsIncrementalLinking());
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
std::string ProjectAdapterVCXProj::getSubSystem() const
{
	auto kind = m_project.kind();

	// TODO: Support for /driver:WDM (NativeWDM or something)
	// https://docs.microsoft.com/en-us/cpp/build/reference/subsystem-specify-subsystem?view=msvc-160

	if (kind == SourceKind::Executable)
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
StringList ProjectAdapterVCXProj::getSourceExtensions() const
{
	StringList ret{
		"asmx",
		"asm",
		"bat",
		"hpj",
		"idl",
		"odl",
		"def",
		"ixx",
		"cppm",
		"c++",
		"cxx",
		"cc",
		"c",
		"cpp",
	};

	const auto& fileExtensions = m_state.paths.allFileExtensions();
	const auto& resourceExtensions = m_state.paths.resourceExtensions();

	for (auto& ext : fileExtensions)
	{
		if (String::equals(resourceExtensions, ext))
			continue;

		List::addIfDoesNotExist(ret, ext);
	}

	// MS defaults
	std::reverse(ret.begin(), ret.end());

	return ret;
}

/*****************************************************************************/
StringList ProjectAdapterVCXProj::getHeaderExtensions() const
{
	// MS defaults
	return {
		"h",
		"hh",
		"hpp",
		"hxx",
		"h++",
		"hm",
		"inl",
		"inc",
		"ipp",
		"xsd",
	};
}

/*****************************************************************************/
StringList ProjectAdapterVCXProj::getResourceExtensions() const
{
	// MS defaults
	return {
		"rc",
		"ico",
		"cur",
		"bmp",
		"dlg",
		"rc2",
		"rct",
		"bin",
		"rgs",
		"gif",
		"jpg",
		"jpeg",
		"jpe",
		"resx",
		"tiff",
		"tif",
		"png",
		"wav",
		"mfcribbon-ms",
	};
}

}
