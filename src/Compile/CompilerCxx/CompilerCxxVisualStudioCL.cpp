/*
	Distributed under the OSI-approved BSD 3-Clause License.
	See accompanying file LICENSE.txt for details.
*/

#include "Compile/CompilerCxx/CompilerCxxVisualStudioCL.hpp"

#include "Compile/Environment/ICompileEnvironment.hpp"
#include "State/BuildConfiguration.hpp"
#include "State/BuildInfo.hpp"
#include "State/BuildPaths.hpp"
#include "State/BuildState.hpp"
#include "State/CompilerTools.hpp"
#include "State/Target/SourceTarget.hpp"
#include "Terminal/Commands.hpp"
#include "Terminal/Environment.hpp"
#include "Terminal/Path.hpp"
#include "Utility/List.hpp"
#include "Utility/String.hpp"

namespace chalet
{
/*****************************************************************************/
CompilerCxxVisualStudioCL::CompilerCxxVisualStudioCL(const BuildState& inState, const SourceTarget& inProject) :
	ICompilerCxx(inState, inProject),
	m_msvcAdapter(inState, inProject)
{
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::initialize()
{
	if (!configureWarnings())
		return false;

	if (!createPrecompiledHeaderSource())
		return false;

	if (m_project.cppModules())
	{
		auto toolsDir = Environment::getAsString("VCToolsInstallDir");
		Path::sanitize(toolsDir);

		std::string arch{ "x64" };
		if (m_state.info.hostArchitecture() == Arch::Cpu::ARM64)
			arch = "arm64";

		m_ifcDirectory = fmt::format("{}/ifc/{}", toolsDir, arch);
	}

	return true;
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::createPrecompiledHeaderSource()
{
	if (m_state.toolchain.strategy() == StrategyType::MSBuild)
		return true; // created by the project exporter

	std::string path = m_state.paths.objDir();
	path.push_back('/');

	return m_msvcAdapter.createPrecompiledHeaderSource(path, path);
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::configureWarnings()
{
	m_warningFlag = getWarningLevel(m_msvcAdapter.getWarningLevel());

	return !m_warningFlag.empty();
}

/*****************************************************************************/
StringList CompilerCxxVisualStudioCL::getPrecompiledHeaderCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const std::string& arch)
{
	UNUSED(generateDependency, dependency, arch);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	auto pchObject = m_state.paths.getPrecompiledHeaderObject(outputFile);

	ret.emplace_back(getQuotedPath(executable));
	ret.emplace_back("/nologo");
	ret.emplace_back("/c");
	addCharsets(ret);

	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	if (generateDependency && isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addSourceFileInterpretation(ret, SourceType::CxxPrecompiledHeader);
	addLanguageStandard(ret, SourceType::CxxPrecompiledHeader);
	// addCppCoroutines(ret);

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addAdditionalOptions(ret);
		addNativeJustMyCodeDebugging(ret);
		addWarnings(ret);
		addDiagnostics(ret);
		addAdditionalSecurityChecks(ret);
		addOptimizations(ret);
		addGenerateIntrinsicFunctions(ret);
		addWholeProgramOptimization(ret);
		addDefines(ret);
		// /Gm-  // deprecated
		addNoExceptionsOption(ret);
		addRuntimeErrorChecks(ret);
		addThreadModelCompileOption(ret);
		addBufferSecurityCheck(ret);
		addFunctionLevelLinking(ret);
		addFastMathOption(ret);
		addStandardsConformance(ret);
		addStandardBehaviors(ret);
		addProgramDatabaseOutput(ret);
		addExternalWarnings(ret);
		addCallingConvention(ret);
		// addFullPathSourceCode(ret);
	}

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	ret.emplace_back(getPathCommand("/Fp", outputFile));
	ret.emplace_back(getPathCommand("/Yc", m_msvcAdapter.pchMinusLocation()));
	ret.emplace_back(getPathCommand("/Fo", pchObject));

	UNUSED(inputFile);

	ret.emplace_back(getQuotedPath(m_msvcAdapter.pchSource()));

	return ret;
}

/*****************************************************************************/
StringList CompilerCxxVisualStudioCL::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const SourceType derivative)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedPath(executable));
	ret.emplace_back("/nologo");
	ret.emplace_back("/c");
	addCharsets(ret);
	// ret.emplace_back("/MP");

	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	if (generateDependency && isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addSourceFileInterpretation(ret, derivative);
	addLanguageStandard(ret, derivative);
	// addCppCoroutines(ret);

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addAdditionalOptions(ret);
		addNativeJustMyCodeDebugging(ret);
		addWarnings(ret);
		addDiagnostics(ret);
		addAdditionalSecurityChecks(ret);
		addOptimizations(ret);
		addGenerateIntrinsicFunctions(ret);
		addWholeProgramOptimization(ret);
		addDefines(ret);
		// /Gm-  // deprecated
		addNoExceptionsOption(ret);
		addRuntimeErrorChecks(ret);
		addThreadModelCompileOption(ret);
		addBufferSecurityCheck(ret);
		addFastMathOption(ret);
		addFunctionLevelLinking(ret);
		addStandardsConformance(ret);
		addStandardBehaviors(ret);
		addProgramDatabaseOutput(ret);
		addExternalWarnings(ret);
		addCallingConvention(ret);
		// addFullPathSourceCode(ret);
	}

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	addPchInclude(ret, derivative);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.emplace_back(getQuotedPath(inputFile));

	return ret;
}

/*****************************************************************************/
StringList CompilerCxxVisualStudioCL::getModuleCommand(const std::string& inputFile, const std::string& outputFile, const std::string& dependencyFile, const std::string& interfaceFile, const StringList& inModuleReferences, const StringList& inHeaderUnits, const ModuleFileType inType)
{
	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty() || dependencyFile.empty() || interfaceFile.empty())
		return ret;

	bool isDependency = inType == ModuleFileType::ModuleDependency || inType == ModuleFileType::HeaderUnitDependency;
	bool isHeaderUnit = inType == ModuleFileType::HeaderUnitObject || inType == ModuleFileType::HeaderUnitDependency;

	ret.emplace_back(executable);
	ret.emplace_back("/nologo");
	ret.emplace_back("/c");
	addCharsets(ret);
	// ret.emplace_back("/MP");

	addSourceFileInterpretation(ret, SourceType::CPlusPlus);
	addLanguageStandard(ret, SourceType::CPlusPlus);
	// addCppCoroutines(ret);

	ret.emplace_back("/experimental:module");
	/*if (!isDependency)
	{
		ret.emplace_back("/ifcSearchDir");
		ret.emplace_back(m_state.paths.objDir());
	}*/

	ret.emplace_back("/stdIfcDir");
	ret.emplace_back(getQuotedPath(m_ifcDirectory));

	if (inType != ModuleFileType::ModuleImplementationUnit)
	{
		ret.emplace_back("/ifcOutput");
		ret.emplace_back(getQuotedPath(interfaceFile));
	}

	if (isDependency)
		ret.emplace_back("/sourceDependencies:directives");
	else
		ret.emplace_back("/sourceDependencies");

	ret.emplace_back(getQuotedPath(dependencyFile));

	if (isHeaderUnit)
		ret.emplace_back("/exportHeader");
	else if (inType != ModuleFileType::ModuleImplementationUnit)
		ret.emplace_back("/interface");

	for (const auto& item : inModuleReferences)
	{
		ret.emplace_back("/reference");
		ret.emplace_back(getQuotedPath(item));
	}

	for (const auto& item : inHeaderUnits)
	{
		ret.emplace_back("/headerUnit");
		ret.emplace_back(getQuotedPath(item));
	}

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addAdditionalOptions(ret);
		addNativeJustMyCodeDebugging(ret);
		addWarnings(ret);
		addDiagnostics(ret);
		addAdditionalSecurityChecks(ret);
		addOptimizations(ret);
		addGenerateIntrinsicFunctions(ret);
		addWholeProgramOptimization(ret);
		addDefines(ret);
		// /Gm-  // deprecated
		addNoExceptionsOption(ret);
		addRuntimeErrorChecks(ret);
		addThreadModelCompileOption(ret);
		addBufferSecurityCheck(ret);
		addFastMathOption(ret);
		addFunctionLevelLinking(ret);
		addStandardsConformance(ret);
		addStandardBehaviors(ret);
		addProgramDatabaseOutput(ret);
		addExternalWarnings(ret);
		addCallingConvention(ret);
		// addFullPathSourceCode(ret);
	}

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	// addPchInclude(ret, SourceType::CPlusPlus);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.emplace_back(getQuotedPath(inputFile));

	return ret;
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::getCommandOptions(StringList& outArgList, const SourceType derivative)
{
	outArgList.emplace_back("/c");
	addCharsets(outArgList);
	// outArgList.emplace_back("/MP");

	addSourceFileInterpretation(outArgList, derivative);
	addLanguageStandard(outArgList, derivative);
	// addCppCoroutines(outArgList);

	addCompileOptions(outArgList);

	{
		addSeparateProgramDatabase(outArgList);
		addAdditionalOptions(outArgList);
		addNativeJustMyCodeDebugging(outArgList);
		addWarnings(outArgList);
		addDiagnostics(outArgList);
		addAdditionalSecurityChecks(outArgList);
		addOptimizations(outArgList);
		addGenerateIntrinsicFunctions(outArgList);
		addWholeProgramOptimization(outArgList);

		// addDefines(outArgList);

		// /Gm-  // deprecated
		addNoExceptionsOption(outArgList);
		addRuntimeErrorChecks(outArgList);
		addThreadModelCompileOption(outArgList);
		addBufferSecurityCheck(outArgList);
		addFastMathOption(outArgList);
		addFunctionLevelLinking(outArgList);
		addStandardsConformance(outArgList);
		addStandardBehaviors(outArgList);
		addProgramDatabaseOutput(outArgList);
		addExternalWarnings(outArgList);
		addCallingConvention(outArgList);
		// addFullPathSourceCode(outArgList);
	}

	addSanitizerOptions(outArgList);
	addNoRunTimeTypeInformationOption(outArgList);
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addSourceFileInterpretation(StringList& outArgList, const SourceType derivative) const
{
	const CodeLanguage language = m_project.language();
	if (derivative == SourceType::CPlusPlus || (derivative == SourceType::CxxPrecompiledHeader && language == CodeLanguage::CPlusPlus))
	{
		List::addIfDoesNotExist(outArgList, "/TP"); // Treat code as C++
	}
	else if (derivative == SourceType::C || (derivative == SourceType::CxxPrecompiledHeader && language == CodeLanguage::C))
	{
		List::addIfDoesNotExist(outArgList, "/TC"); // Treat code as C
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addIncludes(StringList& outArgList) const
{
	// List::addIfDoesNotExist(outArgList, "/X"); // ignore "Path"

	const std::string option{ "/I" };
	auto includes = m_msvcAdapter.getIncludeDirectories();
	for (const auto& dir : includes)
	{
		outArgList.emplace_back(getPathCommand(option, dir));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addWarnings(StringList& outArgList) const
{
	if (!m_warningFlag.empty())
		List::addIfDoesNotExist(outArgList, fmt::format("/{}", m_warningFlag));

	if (m_project.treatWarningsAsErrors())
		List::addIfDoesNotExist(outArgList, "/WX");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addDefines(StringList& outArgList) const
{
	bool isNative = m_state.toolchain.strategy() == StrategyType::Native;
	const std::string prefix{ "/D" };
	for (auto& define : m_project.defines())
	{
		auto pos = define.find("=\"");
		if (!isNative && pos != std::string::npos && define.back() == '\"')
		{
			std::string key = define.substr(0, pos);
			std::string value = define.substr(pos + 2, define.size() - (key.size() + 3));
			std::string def = fmt::format("{}=\\\"{}\\\"", key, value);
			outArgList.emplace_back(prefix + def);
		}
		else
		{
			outArgList.emplace_back(prefix + define);
		}
	}

	if (!m_msvcAdapter.supportsExceptions())
		List::addIfDoesNotExist(outArgList, prefix + "_HAS_EXCEPTIONS=0");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addPchInclude(StringList& outArgList, const SourceType derivative) const
{
	UNUSED(derivative);

	if (m_project.usesPrecompiledHeader())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project);

		outArgList.emplace_back(getPathCommand("/Yu", m_msvcAdapter.pchMinusLocation()));

		// /Fp specifies the location of the PCH object file
		outArgList.emplace_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		outArgList.emplace_back(getPathCommand("/FI", m_msvcAdapter.pchMinusLocation()));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addOptimizations(StringList& outArgList) const
{
	char opt = m_msvcAdapter.getOptimizationLevel();
	if (opt > 0)
		List::addIfDoesNotExist(outArgList, fmt::format("/O{}", opt));

	char inlineOpt = m_msvcAdapter.getInlineFuncExpansion();
	if (inlineOpt > 0)
		List::addIfDoesNotExist(outArgList, fmt::format("/Ob{}", inlineOpt));
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addLanguageStandard(StringList& outArgList, const SourceType derivative) const
{
	// https://docs.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-160
	// https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B

	std::string standard;

	const CodeLanguage language = m_project.language();
	if (derivative == SourceType::CPlusPlus || (derivative == SourceType::CxxPrecompiledHeader && language == CodeLanguage::CPlusPlus))
	{
		standard = m_msvcAdapter.getLanguageStandardCpp();
	}
	else if (derivative == SourceType::C || (derivative == SourceType::CxxPrecompiledHeader && language == CodeLanguage::C))
	{
		standard = m_msvcAdapter.getLanguageStandardC();
	}

	if (!standard.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/std:{}", standard));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCompileOptions(StringList& outArgList) const
{
	for (auto& option : m_project.compileOptions())
	{
		outArgList.emplace_back(option);
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCharsets(StringList& outArgList) const
{
	outArgList.emplace_back(fmt::format("/source-charset:{}", m_project.inputCharset()));
	outArgList.emplace_back(fmt::format("/execution-charset:{}", m_project.executionCharset()));
	outArgList.emplace_back("/validate-charset");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	// must also disable rtti for no exceptions
	if (m_msvcAdapter.disableRunTimeTypeInformation())
		List::addIfDoesNotExist(outArgList, "/GR-");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNoExceptionsOption(StringList& outArgList) const
{
	// /EH - Exception handling model
	// s - standard C++ stack unwinding
	// c - functions declared as extern "C" never throw

	if (m_msvcAdapter.supportsExceptions())
		List::addIfDoesNotExist(outArgList, "/EHa");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addFastMathOption(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsFastMath())
		List::addIfDoesNotExist(outArgList, "/fp:fast");
	else
		List::addIfDoesNotExist(outArgList, "/fp:precise");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addThreadModelCompileOption(StringList& outArgList) const
{
	// /MD - dynamically links with MSVCRT.lib
	// /MDd - dynamically links with MSVCRTD.lib (debug version)

	// /MT - statically links with LIBCMT.lib
	// /MTd - statically links with LIBCMTD.lib (debug version)

	std::string flag;
	auto type = m_msvcAdapter.getRuntimeLibraryType();
	switch (type)
	{
		case WindowsRuntimeLibraryType::MultiThreadedDebug:
			flag = "/MTd";
			break;
		case WindowsRuntimeLibraryType::MultiThreadedDebugDLL:
			flag = "/MDd";
			break;
		case WindowsRuntimeLibraryType::MultiThreadedDLL:
			flag = "/MD";
			break;
		case WindowsRuntimeLibraryType::MultiThreaded:
		default:
			flag = "/MT";
			break;
	}

	chalet_assert(!flag.empty(), "");
	List::addIfDoesNotExist(outArgList, std::move(flag));
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addSanitizerOptions(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsAddressSanitizer())
		List::addIfDoesNotExist(outArgList, "/fsanitize=address");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addDiagnostics(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/diagnostics:caret");
	// List::addIfDoesNotExist(outArgList, "/diagnostics:column");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addWholeProgramOptimization(StringList& outArgList) const
{
	// NOTE: Can't use dumpbin with .obj files compiled with /GL

	// Required by LINK's Link-time code generation (/LTCG)
	// Basically ends up being quicker compiler times for a slower link time, remedied further by incremental linking
	if (m_msvcAdapter.supportsWholeProgramOptimization())
		List::addIfDoesNotExist(outArgList, "/GL");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNativeJustMyCodeDebugging(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsJustMyCodeDebugging())
		List::addIfDoesNotExist(outArgList, "/JMC");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addBufferSecurityCheck(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsBufferSecurityCheck())
		List::addIfDoesNotExist(outArgList, "/GS");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addStandardBehaviors(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsTreatWChartAsBuiltInType())
		List::addIfDoesNotExist(outArgList, "/Zc:wchar_t"); // wchar_t is native type

	if (m_msvcAdapter.supportsForceConformanceInForLoopScope())
		List::addIfDoesNotExist(outArgList, "/Zc:forScope");

	if (m_msvcAdapter.supportsRemoveUnreferencedCodeData())
		List::addIfDoesNotExist(outArgList, "/Zc:inline");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addAdditionalSecurityChecks(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsSDLCheck())
		List::addIfDoesNotExist(outArgList, "/sdl");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCallingConvention(StringList& outArgList) const
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
			flag = "/Gd";
			break;
		case WindowsCallingConvention::FastCall:
			flag = "/Gr";
			break;
		case WindowsCallingConvention::StdCall:
			flag = "/Gz";
			break;
		case WindowsCallingConvention::VectorCall:
		default:
			flag = "/Gv";
			break;
	}

	List::addIfDoesNotExist(outArgList, std::move(flag));
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addFullPathSourceCode(StringList& outArgList) const
{
	// full path of source code file
	// List::addIfDoesNotExist(outArgList, "/FC");
	UNUSED(outArgList);
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addStandardsConformance(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsConformanceMode())
		List::addIfDoesNotExist(outArgList, "/permissive-"); // standards conformance
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addSeparateProgramDatabase(StringList& outArgList) const
{
	/*
		/ZI - separate pdb w/ Edit & Continue
		/Zi - separate pdb
	*/

	if (m_msvcAdapter.supportsEditAndContinue())
		List::addIfDoesNotExist(outArgList, "/ZI");
	else
		List::addIfDoesNotExist(outArgList, "/Zi");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addAdditionalOptions(StringList& outArgList) const
{
	auto options = m_msvcAdapter.getAdditionalCompilerOptions();
	for (auto&& option : options)
	{
		List::addIfDoesNotExist(outArgList, std::move(option));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addProgramDatabaseOutput(StringList& outArgList) const
{
	auto pdbOutput = fmt::format("{}/vc{}.pdb", m_state.paths.objDir(), m_msvcAdapter.getPlatformToolset());
	outArgList.emplace_back(getPathCommand("/Fd", pdbOutput)); // PDB output
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addExternalWarnings(StringList& outArgList) const
{
	if (!m_warningFlag.empty() && m_msvcAdapter.supportsExternalWarnings())
	{
		if (m_versionMajorMinor < 1929) // requires experimental
		{
			List::addIfDoesNotExist(outArgList, "/experimental:external");
		}

		List::addIfDoesNotExist(outArgList, fmt::format("/external:{}", m_warningFlag));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addRuntimeErrorChecks(StringList& outArgList) const
{
	// Enables stack frame run-time error checking, uninitialized variables
	if (m_msvcAdapter.supportsRunTimeErrorChecks())
		List::addIfDoesNotExist(outArgList, "/RTC1");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addFunctionLevelLinking(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsFunctionLevelLinking())
		List::addIfDoesNotExist(outArgList, "/Gy");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addGenerateIntrinsicFunctions(StringList& outArgList) const
{
	if (m_msvcAdapter.supportsGenerateIntrinsicFunctions())
		List::addIfDoesNotExist(outArgList, "/Oi");
}

/*****************************************************************************/
// Moved to CommandAdapterMSVC::getAdditionalCompilerOptions
/*void CompilerCxxVisualStudioCL::addCppCoroutines(StringList& outArgList) const
{
	if (m_project.cppCoroutines())
	{
		if (m_versionMajorMinor >= 1929)
		{
			List::addIfDoesNotExist(outArgList, "/await:strict");
		}
		else
		{
			List::addIfDoesNotExist(outArgList, "/await");
		}
	}
}*/

/*****************************************************************************/
std::string CompilerCxxVisualStudioCL::getWarningLevel(const MSVCWarningLevel inLevel) const
{
	switch (inLevel)
	{
		case MSVCWarningLevel::Level1:
			return "W1";

		case MSVCWarningLevel::Level2:
			return "W2";

		case MSVCWarningLevel::Level3:
			return "W3";

		case MSVCWarningLevel::Level4:
			return "W4";

		case MSVCWarningLevel::LevelAll:
			return "Wall";

		default:
			break;
	}

	return std::string();
}

}
