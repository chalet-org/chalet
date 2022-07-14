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
	ICompilerCxx(inState, inProject)
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
		m_ifcDirectory = fmt::format("{}/ifc/x64", toolsDir);
	}

	return true;
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::createPrecompiledHeaderSource()
{
	const auto& cxxExt = m_state.paths.cxxExtension();
	if (m_project.usesPrecompiledHeader() && !cxxExt.empty())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.precompiledHeader();
		m_pchSource = fmt::format("{}/{}.{}", objDir, pch, cxxExt);

		m_pchMinusLocation = String::getPathFilename(pch);

		if (!Commands::pathExists(m_pchSource))
		{
			if (!Commands::createFileWithContents(m_pchSource, fmt::format("#include \"{}\"", m_pchMinusLocation)))
				return false;
		}
	}

	return true;
}

/*****************************************************************************/
bool CompilerCxxVisualStudioCL::configureWarnings()
{
	auto level = m_project.getMSVCWarningLevel();
	switch (level)
	{
		case MSVCWarningLevel::Level1:
			m_warningFlag = "W1";
			break;

		case MSVCWarningLevel::Level2:
			m_warningFlag = "W2";
			break;

		case MSVCWarningLevel::Level3:
			m_warningFlag = "W3";
			break;

		case MSVCWarningLevel::Level4:
			m_warningFlag = "W4";
			break;

		case MSVCWarningLevel::LevelAll:
			m_warningFlag = "Wall";
			break;

		default:
			m_warningFlag.clear();
			break;
	}
	return true;
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

	std::string pchObject = outputFile;
	String::replaceAll(pchObject, ".pch", ".obj");

	const auto specialization = m_project.language() == CodeLanguage::CPlusPlus ? CxxSpecialization::CPlusPlus : CxxSpecialization::C;

	ret.emplace_back(getQuotedExecutablePath(executable));
	ret.emplace_back("/nologo");
	ret.emplace_back("/c");
	addCharsets(ret);

	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	if (generateDependency && isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addLanguageStandard(ret, specialization);
	addCppCoroutines(ret);

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addForceSeparateProgramDatabaseWrites(ret);
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

	// addInlineFunctionExpansion(ret);
	// addUnsortedOptions(ret);

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	ret.emplace_back(getPathCommand("/Fp", outputFile));
	ret.emplace_back(getPathCommand("/Yc", m_pchMinusLocation));
	ret.emplace_back(getPathCommand("/Fo", pchObject));

	UNUSED(inputFile);

	ret.push_back(m_pchSource);

	return ret;
}

/*****************************************************************************/
StringList CompilerCxxVisualStudioCL::getCommand(const std::string& inputFile, const std::string& outputFile, const bool generateDependency, const std::string& dependency, const CxxSpecialization specialization)
{
	UNUSED(generateDependency, dependency);

	chalet_assert(!outputFile.empty(), "");

	StringList ret;

	auto& executable = m_state.toolchain.compilerCxx(m_project.language()).path;

	if (executable.empty())
		return ret;

	ret.emplace_back(getQuotedExecutablePath(executable));
	ret.emplace_back("/nologo");
	ret.emplace_back("/c");
	addCharsets(ret);
	// ret.emplace_back("/MP");

	const bool isNinja = m_state.toolchain.strategy() == StrategyType::Ninja;
	if (generateDependency && isNinja)
	{
		ret.emplace_back("/showIncludes");
	}

	addLanguageStandard(ret, specialization);
	addCppCoroutines(ret);

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addForceSeparateProgramDatabaseWrites(ret);
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

	// addInlineFunctionExpansion(ret);
	// addUnsortedOptions(ret);

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	addPchInclude(ret);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

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

	addLanguageStandard(ret, CxxSpecialization::CPlusPlus);
	addCppCoroutines(ret);

	ret.emplace_back("/experimental:module");
	/*if (!isDependency)
	{
		ret.emplace_back("/ifcSearchDir");
		ret.emplace_back(m_state.paths.objDir());
	}*/

	ret.emplace_back("/stdIfcDir");
	ret.emplace_back(m_ifcDirectory);

	if (inType != ModuleFileType::ModuleImplementationUnit)
	{
		ret.emplace_back("/ifcOutput");
		ret.emplace_back(interfaceFile);
	}

	if (isDependency)
		ret.emplace_back("/sourceDependencies:directives");
	else
		ret.emplace_back("/sourceDependencies");

	ret.emplace_back(dependencyFile);

	if (isHeaderUnit)
		ret.emplace_back("/exportHeader");
	else if (inType != ModuleFileType::ModuleImplementationUnit)
		ret.emplace_back("/interface");

	for (const auto& item : inModuleReferences)
	{
		ret.emplace_back("/reference");
		ret.emplace_back(item);
	}

	for (const auto& item : inHeaderUnits)
	{
		ret.emplace_back("/headerUnit");
		ret.emplace_back(item);
	}

	addCompileOptions(ret);

	{
		addSeparateProgramDatabase(ret);
		addForceSeparateProgramDatabaseWrites(ret);
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

	// addInlineFunctionExpansion(ret);
	// addUnsortedOptions(ret);

	addSanitizerOptions(ret);
	addNoRunTimeTypeInformationOption(ret);
	addIncludes(ret);

	// addPchInclude(ret);

	ret.emplace_back(getPathCommand("/Fo", outputFile));

	ret.push_back(inputFile);

	return ret;
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::getCommandOptions(StringList& outArgList, const CxxSpecialization specialization)
{
	outArgList.emplace_back("/c");
	addCharsets(outArgList);
	// outArgList.emplace_back("/MP");

	addLanguageStandard(outArgList, specialization);
	addCppCoroutines(outArgList);

	addCompileOptions(outArgList);

	{
		addSeparateProgramDatabase(outArgList);
		addForceSeparateProgramDatabaseWrites(outArgList);
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

	// addInlineFunctionExpansion(outArgList);
	// addUnsortedOptions(outArgList);

	addSanitizerOptions(outArgList);
	addNoRunTimeTypeInformationOption(outArgList);
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addIncludes(StringList& outArgList) const
{
	// List::addIfDoesNotExist(outArgList, "/X"); // ignore "Path"

	const std::string option{ "/I" };

	for (const auto& dir : m_project.includeDirs())
	{
		std::string outDir = dir;
		if (String::endsWith('/', outDir))
			outDir.pop_back();

		outArgList.emplace_back(getPathCommand(option, outDir));
	}

	if (m_project.usesPrecompiledHeader())
	{
		auto outDir = String::getPathFolder(m_project.precompiledHeader());
		outArgList.emplace_back(getPathCommand(option, outDir));
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
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addPchInclude(StringList& outArgList) const
{
	if (m_project.usesPrecompiledHeader())
	{
		const auto objDirPch = m_state.paths.getPrecompiledHeaderTarget(m_project);

		outArgList.emplace_back(getPathCommand("/Yu", m_pchMinusLocation));

		// /Fp specifies the location of the PCH object file
		outArgList.emplace_back(getPathCommand("/Fp", objDirPch));

		// /FI force-includes the PCH source file so one doesn't need to use the #include directive in every file
		outArgList.emplace_back(getPathCommand("/FI", m_pchMinusLocation));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addOptimizations(StringList& outArgList) const
{
	std::string opt;
	std::string inlineOpt;

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
		// force -O0 (anything else would be in error)
		opt = "/Od";
		inlineOpt = "/Ob0";
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				opt = "/O1";
				if (m_state.configuration.debugSymbols())
					inlineOpt = "/Ob1";
				else
					inlineOpt = "/Ob2";
				break;

			case OptimizationLevel::L2:
				opt = "/O2";
				if (m_state.configuration.debugSymbols())
					inlineOpt = "/Ob1";
				else
					inlineOpt = "/Ob2";
				break;

			case OptimizationLevel::L3:
				opt = "/O2";
				if (m_versionMajorMinor >= 1920) // VS 2019+
					inlineOpt = "/Ob3";
				else
					inlineOpt = "/Ob2";
				break;

			case OptimizationLevel::Size:
				opt = "/Os";
				inlineOpt = "/Ob1";
				break;

			case OptimizationLevel::Fast:
				opt = "/Ot";
				if (m_versionMajorMinor >= 1920) // VS 2019+
					inlineOpt = "/Ob3";
				else
					inlineOpt = "/Ob2";
				break;

			case OptimizationLevel::Debug:
			case OptimizationLevel::None:
				opt = "/Od";
				inlineOpt = "/Ob0";
				break;

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	if (!opt.empty())
	{
		List::addIfDoesNotExist(outArgList, std::move(opt));
		List::addIfDoesNotExist(outArgList, std::move(inlineOpt));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	// https://docs.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-160
	// https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B

	if (specialization == CxxSpecialization::C)
	{
		List::addIfDoesNotExist(outArgList, "/TC"); // Treat code as C

		// C standards conformance was added in 2019 16.8
		if (m_versionMajorMinor >= 1928)
		{
			std::string langStandard = String::toLowerCase(m_project.cStandard());
			String::replaceAll(langStandard, "gnu", "");
			String::replaceAll(langStandard, "c", "");
			if (String::equals(StringList{ "2x", "18", "17", "iso9899:2018", "iso9899:2017" }, langStandard))
			{
				List::addIfDoesNotExist(outArgList, "/std:c17");
			}
			else
			{
				List::addIfDoesNotExist(outArgList, "/std:c11");
			}
		}
	}
	else if (specialization == CxxSpecialization::CPlusPlus)
	{
		List::addIfDoesNotExist(outArgList, "/TP"); // Treat code as C++

		// 2015 Update 3 or later (/std flag doesn't exist prior
		if (m_versionMajorMinor > 1900 || (m_versionMajorMinor == 1900 && m_versionPatch >= 24210))
		{
			bool set = false;
			std::string langStandard = String::toLowerCase(m_project.cppStandard());
			String::replaceAll(langStandard, "gnu++", "");
			String::replaceAll(langStandard, "c++", "");

			if (String::equals(StringList{ "20", "2a" }, langStandard))
			{
				if (m_versionMajorMinor >= 1929)
				{
					List::addIfDoesNotExist(outArgList, "/std:c++20");
					set = true;
				}
			}
			else if (String::equals(StringList{ "17", "1z" }, langStandard))
			{
				if (m_versionMajorMinor >= 1911)
				{
					List::addIfDoesNotExist(outArgList, "/std:c++17");
					set = true;
				}
			}
			else if (String::equals(StringList{ "14", "1y", "11", "0x", "03", "98" }, langStandard))
			{
				// Note: There was never "/std:c++11", "/std:c++03" or "/std:c++98"
				List::addIfDoesNotExist(outArgList, "/std:c++14");
				set = true;
			}

			if (!set)
			{
				List::addIfDoesNotExist(outArgList, "/std:c++latest");
			}
		}
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
	if (!m_project.runtimeTypeInformation() || !m_project.exceptions())
	{
		List::addIfDoesNotExist(outArgList, "/GR-");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNoExceptionsOption(StringList& outArgList) const
{
	// /EH - Exception handling model
	// s - standard C++ stack unwinding
	// c - functions declared as extern "C" never throw

	if (!m_project.exceptions())
	{
		List::addIfDoesNotExist(outArgList, "/D_HAS_EXCEPTIONS=0");
	}
	else
	{
		List::addIfDoesNotExist(outArgList, "/EHsc");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addFastMathOption(StringList& outArgList) const
{
	if (m_project.fastMath())
	{
		List::addIfDoesNotExist(outArgList, "/fp:fast");
	}
	else
	{
		List::addIfDoesNotExist(outArgList, "/fp:precise");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addThreadModelCompileOption(StringList& outArgList) const
{
	// /MD - dynamically links with MSVCRT.lib
	// /MDd - dynamically links with MSVCRTD.lib (debug version)

	// /MT - statically links with LIBCMT.lib
	// /MTd - statically links with LIBCMTD.lib (debug version)

	if (m_project.staticRuntimeLibrary())
	{
		// Note: This will generate a larger binary!
		//
		if (m_state.configuration.debugSymbols())
			List::addIfDoesNotExist(outArgList, "/MTd");
		else
			List::addIfDoesNotExist(outArgList, "/MT");
	}
	else
	{
		if (m_state.configuration.debugSymbols())
			List::addIfDoesNotExist(outArgList, "/MDd");
		else
			List::addIfDoesNotExist(outArgList, "/MD");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addSanitizerOptions(StringList& outArgList) const
{
	if (m_versionMajorMinor >= 1928 && m_state.configuration.sanitizeAddress())
	{
		List::addIfDoesNotExist(outArgList, "/fsanitize=address");
	}
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
	if (m_state.configuration.interproceduralOptimization())
	{
		outArgList.emplace_back("/GL");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addNativeJustMyCodeDebugging(StringList& outArgList) const
{
	if (m_state.configuration.debugSymbols())
	{
		List::addIfDoesNotExist(outArgList, "/JMC");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addBufferSecurityCheck(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/GS");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addStandardBehaviors(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/Zc:wchar_t"); // wchar_t is native type
	List::addIfDoesNotExist(outArgList, "/Zc:inline");
	List::addIfDoesNotExist(outArgList, "/Zc:forScope");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addAdditionalSecurityChecks(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/sdl");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCallingConvention(StringList& outArgList) const
{
	// default calling convention
	List::addIfDoesNotExist(outArgList, "/Gd");
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
	if (m_versionMajorMinor >= 1910) // VS 2017+
	{
		List::addIfDoesNotExist(outArgList, "/permissive-"); // standards conformance
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addSeparateProgramDatabase(StringList& outArgList) const
{
	/*
		/ZI - separate pdb w/ Edit & Continue
		/Zi - separate pdb
	*/

	const auto arch = m_state.info.targetArchitecture();
	if (m_state.configuration.debugSymbols()
		&& !m_state.configuration.enableSanitizers()
		&& !m_state.configuration.enableProfiling()
		&& (arch == Arch::Cpu::X64 || arch == Arch::Cpu::X86))
	{
		List::addIfDoesNotExist(outArgList, "/ZI");
	}
	else
	{
		List::addIfDoesNotExist(outArgList, "/Zi");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addForceSeparateProgramDatabaseWrites(StringList& outArgList) const
{
	List::addIfDoesNotExist(outArgList, "/FS");
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addProgramDatabaseOutput(StringList& outArgList) const
{
	auto buildDir = m_state.paths.buildOutputDir() + '/';
	if (m_state.configuration.debugSymbols())
	{
		outArgList.emplace_back(getPathCommand("/Fd", buildDir)); // PDB output
	}
	else
	{
		outArgList.emplace_back(getPathCommand("/Fd", buildDir)); // PDB output
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addExternalWarnings(StringList& outArgList) const
{
	if (!m_warningFlag.empty())
	{
		if (m_versionMajorMinor >= 1913) // added in 15.6
		{
			if (m_versionMajorMinor < 1929) // requires experimental
			{
				List::addIfDoesNotExist(outArgList, "/experimental:external");
			}

			List::addIfDoesNotExist(outArgList, fmt::format("/external:{}", m_warningFlag));
		}
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addRuntimeErrorChecks(StringList& outArgList) const
{
	if (m_state.configuration.debugSymbols())
	{
		// Enables stack frame run-time error checking, uninitialized variables
		List::addIfDoesNotExist(outArgList, "/RTC1");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addInlineFunctionExpansion(StringList& outArgList) const
{
	if (m_state.configuration.debugSymbols())
	{
		// disable inline expansion
		List::addIfDoesNotExist(outArgList, "/Ob0");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addFunctionLevelLinking(StringList& outArgList) const
{
	if (!m_state.configuration.debugSymbols())
	{
		// function level linking
		List::addIfDoesNotExist(outArgList, "/Gy");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addGenerateIntrinsicFunctions(StringList& outArgList) const
{
	if (!m_state.configuration.debugSymbols())
	{
		// generate intrinsic functions
		List::addIfDoesNotExist(outArgList, "/Oi");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addCppCoroutines(StringList& outArgList) const
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
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addUnsortedOptions(StringList& outArgList) const
{
	// Note: in MSVC, one can combine these (annoyingly)
	//	Might be desireable to add:
	//    /Oy (suppresses the creation of frame pointers on the call stack for quicker function calls.)

	UNUSED(outArgList);
}

}
