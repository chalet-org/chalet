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
	if (m_project.usesPch())
	{
		const auto& objDir = m_state.paths.objDir();
		const auto& pch = m_project.pch();
		m_pchSource = fmt::format("{}/{}.cpp", objDir, pch);

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
	ret.emplace_back("/utf-8");

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
	ret.emplace_back("/utf-8");
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
	ret.emplace_back("/utf-8");
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

	if (inType != ModuleFileType::ModuleObjectRoot)
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
	else if (inType != ModuleFileType::ModuleObjectRoot)
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

	if (m_project.usesPch())
	{
		auto outDir = String::getPathFolder(m_project.pch());
		outArgList.emplace_back(getPathCommand(option, outDir));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addWarnings(StringList& outArgList) const
{
	m_warningFlag.clear();
	bool warningsAsErrors = false;

	switch (m_project.warningsPreset())
	{
		case ProjectWarningPresets::Minimal:
			m_warningFlag = "W1";
			break;

		case ProjectWarningPresets::Extra:
			m_warningFlag = "W2";
			break;

		case ProjectWarningPresets::Pedantic: {
			m_warningFlag = "W3";
			break;
		}
		case ProjectWarningPresets::Error: {
			m_warningFlag = "W3";
			warningsAsErrors = true;
			break;
		}
		case ProjectWarningPresets::Strict:
		case ProjectWarningPresets::StrictPedantic: {
			m_warningFlag = "W4";
			warningsAsErrors = true;
			break;
		}
		case ProjectWarningPresets::VeryStrict: {
			// m_warningFlag = "Wall"; // Note: Lots of messy compiler level warnings that break your build!
			m_warningFlag = "W4";
			warningsAsErrors = true;
			break;
		}

		case ProjectWarningPresets::Custom: {
			// TODO: Refactor this so the strict warnings are stored somewhere GNU can use
			auto& warnings = m_project.warnings();

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

			bool strictSet = false;
			for (auto& w : warnings)
			{
				if (!String::equals(veryStrict, w))
					continue;

				// m_warningFlag = "Wall";
				m_warningFlag = "W4";
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

					m_warningFlag = "W4";
					strictSet = true;
					break;
				}
			}

			if (!strictSet)
			{
				if (List::contains<std::string>(warnings, "pedantic"))
				{
					m_warningFlag = "W3";
				}
				else if (List::contains<std::string>(warnings, "extra"))
				{
					m_warningFlag = "W2";
				}
				else if (List::contains<std::string>(warnings, "all"))
				{
					m_warningFlag = "W1";
				}
			}

			if (List::contains<std::string>(warnings, "pedantic"))
			{
				warningsAsErrors = true;
			}

			break;
		}

		case ProjectWarningPresets::None:
		default:
			break;
	}

	if (!m_warningFlag.empty())
	{
		List::addIfDoesNotExist(outArgList, fmt::format("/{}", m_warningFlag));

		if (warningsAsErrors)
			List::addIfDoesNotExist(outArgList, "/WX");
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addDefines(StringList& outArgList) const
{
	for (auto& define : m_project.defines())
	{
		outArgList.emplace_back(getPathCommand("/D", define));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addPchInclude(StringList& outArgList) const
{
	// TODO: Potential for more than one pch?
	if (m_project.usesPch())
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

	OptimizationLevel level = m_state.configuration.optimizationLevel();

	if (m_state.configuration.debugSymbols()
		&& level != OptimizationLevel::Debug
		&& level != OptimizationLevel::None
		&& level != OptimizationLevel::CompilerDefault)
	{
		// force -O0 (anything else would be in error)
		opt = "/Od";
	}
	else
	{
		switch (level)
		{
			case OptimizationLevel::L1:
				opt = "/O1";
				break;

			case OptimizationLevel::L2:
				opt = "/O2";
				break;

			case OptimizationLevel::L3:
				opt = "/O2";
				break;

			case OptimizationLevel::Size:
				opt = "/Os";
				break;

			case OptimizationLevel::Fast:
				opt = "/Ot";
				break;

			case OptimizationLevel::Debug:
			case OptimizationLevel::None:
				opt = "/Od";
				break;

			case OptimizationLevel::CompilerDefault:
			default:
				break;
		}
	}

	if (!opt.empty())
	{
		List::addIfDoesNotExist(outArgList, std::move(opt));
	}
}

/*****************************************************************************/
void CompilerCxxVisualStudioCL::addLanguageStandard(StringList& outArgList, const CxxSpecialization specialization) const
{
	// https://docs.microsoft.com/en-us/cpp/build/reference/std-specify-language-standard-version?view=msvc-160
	// https://en.wikipedia.org/wiki/Microsoft_Visual_C%2B%2B

	// TODO: Reverse years so c11 / c++14 is checked explicitly & newest year isn't
	if (specialization == CxxSpecialization::C)
	{
		List::addIfDoesNotExist(outArgList, "/TC"); // Treat code as C

		// C standards conformance was added in 2019 16.8
		if (m_versionMajorMinor >= 1928)
		{
			std::string langStandard = String::toLowerCase(m_project.cStandard());
			String::replaceAll(langStandard, "gnu", "");
			String::replaceAll(langStandard, "c", "");
			if (String::equals({ "2x", "18", "17", "iso9899:2018", "iso9899:2017" }, langStandard))
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
			if (String::equals({ "20", "2a" }, langStandard))
			{
				if (m_versionMajorMinor >= 1929)
				{
					List::addIfDoesNotExist(outArgList, "/std:c++20");
					set = true;
				}
			}
			else if (String::equals({ "17", "1z" }, langStandard))
			{
				if (m_versionMajorMinor >= 1911)
				{
					List::addIfDoesNotExist(outArgList, "/std:c++17");
					set = true;
				}
			}
			else if (String::equals({ "14", "1y", "11", "0x", "03", "98" }, langStandard))
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
void CompilerCxxVisualStudioCL::addNoRunTimeTypeInformationOption(StringList& outArgList) const
{
	// must also disable rtti for no exceptions
	if (!m_project.rtti() || !m_project.exceptions())
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

	if (m_project.staticLinking())
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
	/*if (m_state.configuration.linkTimeOptimization() && !m_state.info.dumpAssembly())
	{
		outArgList.emplace_back("/GL");
	}*/
	UNUSED(outArgList);
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
	if (!m_state.configuration.debugSymbols())
	{
		// this flag is definitely VS 2017+
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
